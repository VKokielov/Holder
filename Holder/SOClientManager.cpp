#include "SOClientManager.h"
#include "AppLibrary.h"

#include "DTConstruct.h"
#include "DTSimple.h"

namespace impl = holder::service;

thread_local 
	holder::lib::LockState impl::SOClientManager::m_lockState{ lib::LockState::Unlocked };

impl::SOClientManager::ProxyMessageReceiver::ProxyMessageReceiver(const std::shared_ptr<impl::IProxy>& pProxy)
	:m_pProxy(pProxy)
{

}

void impl::SOClientManager::ProxyMessageReceiver::OnReceiverReady(messages::DispatchID dispatchId)
{

}

void impl::SOClientManager::ProxyMessageReceiver::OnMessage(const std::shared_ptr<messages::IMessage>& pMsg,
	messages::DispatchID dispatchId)
{
	auto pProxyShared = m_pProxy.lock();

	ISOClientMessage* pRawMessage = nullptr;
#ifdef _DEBUG
	// RTTI!
	pRawMessage = dynamic_cast<ISOClientMessage*>(pMsg.get());

	if (!pRawMessage)
	{
		throw MessageCastException();
	}
#else
	pRawMessage = static_cast<ISOClientMessage*>(pMsg.get());
#endif

	if (pProxyShared)
	{
		pProxyShared->OnClientMessage(*pRawMessage);
	}
}

impl::ClientMgrResult 
impl::SOClientManager::AddMethod(const std::shared_ptr<impl::ISOClientMethod>& pMethod)
{
	// Add a method
	lib::RWLockWrapperWrite lock{ m_mutex, m_lockState };
	
	if (std::find(m_methods.begin(), m_methods.end(), pMethod)
		!= m_methods.end())
	{
		return ClientMgrResult::CantAddDuplicate;
	}

	pMethod->OnAdded(m_methods.size());
	m_methods.emplace_back(pMethod);
	return ClientMgrResult::OK;
}

impl::ClientMgrResult 
impl::SOClientManager::CreateClient(const char* pServiceName,
	const char* pRemoteName,
	const char* pMethodPreference,
	holder::messages::QueueID clientQueue,
	std::shared_ptr<impl::IProxy>& ppProxy)
{
	lib::RWLockWrapperWrite lock{ m_mutex, m_lockState };

	// This function first checks if the service exists in the cache.  If it doesn't,
	// the function tries each method in turn to create it.  The method will call back
	// the object here, and get a service ID.

	CMServiceID serviceID;
	CMService_* pService{ nullptr };
	
	if (FindService(pServiceName, serviceID) == ClientMgrResult::OK)
	{
		auto itService = m_serviceMap.find(serviceID);
		pService = &itService->second;
	}

	std::string methodPreference{ pMethodPreference ? pMethodPreference : "" };

	if (!pService)
	{
		// Use one of the methods to connect
		for (auto& pMethod : m_methods)
		{
			if (methodPreference.empty()
				|| methodPreference == pMethod->GetMethodName())
			{
				if (pMethod->ConnectService(pRemoteName, serviceID)
					== ClientMgrResult::OK)
				{
					// The service was created and should be avaliable.  Try to get it
					auto itService = m_serviceMap.find(serviceID);
					if (itService != m_serviceMap.end())
					{
						pService = &itService->second;
						break;
					}

					// If not, then the function ConnectService lied about being able to create
					// the service.
				}
			}
		}

		if (pService == nullptr)
		{
			return ClientMgrResult::CantConnectService;
		}
	}

	// Find the factory for the proxy in the app library
	std::shared_ptr<base::ITypedAppObjectFactory<IProxy> > proxyFactory;
	{
		std::shared_ptr<base::IAppObjectFactory> untypedProxyFactory;

		if (!base::AppLibrary::GetInstance().FindFactory(pService->args.serviceProxyType.c_str(),
			untypedProxyFactory))
		{
			return ClientMgrResult::ServiceProxyFactoryNotFound;
		}

		proxyFactory = std::dynamic_pointer_cast<base::ITypedAppObjectFactory<IProxy>>
			(untypedProxyFactory);

		if (!proxyFactory)
		{
			// Wrong type
			return ClientMgrResult::ServiceProxyFactoryTypeError;
		}
	}

	// Get the next free client ID from the service
	CMClientID clientID = pService->nextClientID;

	using DTSuite = data::simple::Suite;

	// Create an initializer for the proxy
	auto pProxyInitializer = data::DTDict<DTSuite>(
		{
			data::DictPair("queueID",
				data::DTElem<DTSuite>(clientQueue)),
			data::DictPair("serviceID",
				data::DTElem<DTSuite>(serviceID)),
			data::DictPair("clientID",
				data::DTElem<DTSuite>(clientID))
		}
	);

	// Create the proxy and create a receiver for the proxy
	auto pProxy = proxyFactory->Create(*pProxyInitializer.get());

	if (!pProxy)
	{
		return ClientMgrResult::ServiceProxyNotCreated;
	}

	auto pReceiver = std::make_shared<ProxyMessageReceiver>(pProxy);

	messages::CreateReceiverArgs receiverArgs;
	receiverArgs.pListener = pReceiver;

	messages::ReceiverID receiverID 
		= messages::QueueManager::GetInstance().CreateReceiver(clientQueue, receiverArgs);

	// Create an entry for the client
	pService->nextClientID++;
	auto itClient
		= pService->clientMap.emplace(clientID, Client_()).first;

	// Fill out the client information, inform the corresponding method and return the proxy pointer
	itClient->second.serviceID = serviceID;
	itClient->second.clientID = clientID;
	itClient->second.queueID = clientQueue;
	itClient->second.receiverID = receiverID;

	if (m_methods[pService->args.methodID]->OnNewClient(serviceID, clientID)
		!= ClientMgrResult::OK)
	{
		// This is a fatal failure -- raise it as an exception
		throw ClientCreateException();
	}
	
	ppProxy = pProxy;

}

impl::ClientMgrResult impl::SOClientManager::SendMessage(impl::CMServiceID serviceID,
	impl::CMClientID clientID,
	std::shared_ptr<ISOServiceMessage>& pServiceMessage) const
{
	lib::RWLockWrapperRead lock{ m_mutex, m_lockState };

	auto itService = m_serviceMap.find(serviceID);
	if (itService == m_serviceMap.end())
	{
		return ClientMgrResult::ServiceNotFound;
	}

	auto itClient = itService->second.clientMap.find(clientID);
	if (itClient == itService->second.clientMap.end())
	{
		return ClientMgrResult::ClientNotFound;
	}

	return m_methods[itService->second.args.methodID]->SendMessage(serviceID, clientID, pServiceMessage);
}

impl::ClientMgrResult impl::SOClientManager::OnProxyDestroyed(impl::CMServiceID serviceID,
	impl::CMClientID clientID)
{
	lib::RWLockWrapperWrite lock{ m_mutex, m_lockState };

	auto itService = m_serviceMap.find(serviceID);
	if (itService == m_serviceMap.end())
	{
		return ClientMgrResult::ServiceNotFound;
	}

	auto itClient = itService->second.clientMap.find(clientID);
	if (itClient == itService->second.clientMap.end())
	{
		return ClientMgrResult::ClientNotFound;
	}

	// This proxy no longer exists.  
	messages::QueueManager::GetInstance().RemoveReceiver(itClient->second.queueID,
		itClient->second.receiverID);
	itService->second.clientMap.erase(itClient);

	return m_methods[itService->second.args.methodID]->OnClientDestroyed(serviceID, clientID);
}

impl::ClientMgrResult impl::SOClientManager::FindService(const char* pServiceName,
	impl::CMServiceID& serviceID) const
{
	lib::RWLockWrapperRead lock{ m_mutex, m_lockState };

	lib::PathFromString pfs(pServiceName);
	lib::NodeFinder<CMServiceID> nodeFinder;

	if (lib::TracePath(m_serviceTree, pfs, nodeFinder) != lib::NodeResult::OK)
	{
		return ClientMgrResult::ServiceNotFound;
	}

	serviceID = nodeFinder.GetNodeID();
	return ClientMgrResult::OK;
}

impl::ClientMgrResult impl::SOClientManager::AddService(const impl::CMServiceArgs& args,
	impl::CMServiceID& serviceID)
{
	lib::RWLockWrapperWrite lock{ m_mutex, m_lockState };

	// Create a new service object
	auto nextServiceID = m_freeServiceID++;

	auto itService = m_serviceMap.emplace(std::piecewise_construct,
		std::forward_as_tuple(nextServiceID),
		std::forward_as_tuple()).first;

	itService->second.args = args;
	
	return ClientMgrResult::OK;
}

impl::ClientMgrResult impl::SOClientManager::RemoveService(impl::CMServiceID serviceID)
{
	lib::RWLockWrapperWrite lock{ m_mutex, m_lockState };

	auto itService = m_serviceMap.find(serviceID);
	if (itService == m_serviceMap.end())
	{
		return ClientMgrResult::ServiceNotFound;
	}

	MethodID serviceMethod = itService->second.args.methodID;

	// Destroy the clients
	for (auto& rClient : itService->second.clientMap)
	{
		messages::QueueManager::GetInstance().RemoveReceiver(rClient.second.queueID,
			rClient.second.receiverID);

		m_methods[serviceMethod]->OnClientDestroyed(serviceID, rClient.first);
	}
	
	// Remove the service
	m_serviceMap.erase(itService);

	return ClientMgrResult::OK;
}

impl::ClientMgrResult impl::SOClientManager::DispatchMessageToProxy(impl::CMServiceID serviceID,
	impl::CMClientID clientID,
	std::shared_ptr<impl::ISOClientMessage>& pClientMessage) const
{
	lib::RWLockWrapperRead lock{ m_mutex, m_lockState };


	auto itService = m_serviceMap.find(serviceID);
	if (itService == m_serviceMap.end())
	{
		return ClientMgrResult::ServiceNotFound;
	}

	auto itClient = itService->second.clientMap.find(clientID);
	if (itClient == itService->second.clientMap.end())
	{
		return ClientMgrResult::ClientNotFound;
	}

	std::shared_ptr<messages::IMessage> pMessage{ pClientMessage };

	if (!messages::QueueManager::GetInstance().SendMessage(itClient->second.queueID,
		itClient->second.receiverID, pClientMessage))
	{
		return ClientMgrResult::CantSendMessageToClient;
	}

	return ClientMgrResult::OK;
}

impl::SOClientManager& impl::SOClientManager::GetInstance()
{
	static SOClientManager me;
	return me;
}