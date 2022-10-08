#include "SOServiceManager.h"
#include "RWLockWrapper.h"
#include "PathFromString.h"

namespace impl = holder::service;

impl::SOServiceManager::
ServiceMessageReceiver::ServiceMessageReceiver(const std::shared_ptr<IService>& pService)
	:m_pService(pService)
{

}

void impl::SOServiceManager::ServiceMessageReceiver::OnReceiverReady(messages::DispatchID dispatchId)
{

}
void impl::SOServiceManager::ServiceMessageReceiver::OnMessage(const std::shared_ptr<messages::IMessage>& pMsg,
	messages::DispatchID dispatchId)
{
	// dispatchID is also clientID
	ISOServiceMessage* pRawMessage = nullptr;
#ifdef _DEBUG
	// RTTI!
	pRawMessage = dynamic_cast<ISOServiceMessage*>(pMsg.get());

	if (!pRawMessage)
	{
		throw MessageCastException();
	}
#else
	pRawMessage = static_cast<ISOClientMessage*>(pMsg.get());
#endif

	m_pService->OnServiceMessage(*pRawMessage, (SMClientID)dispatchId);
}

impl::ServiceMgrResult impl::SOServiceManager::AddMethod(const std::shared_ptr<impl::ISOServiceMethod>& pMethod)
{
	lib::RWLockWrapperWrite lock{ m_mutex, m_lockState };

	std::string methodName(pMethod->GetMethodName());
	
	if (std::find(m_methods.begin(), m_methods.end(), pMethod)
		!= m_methods.end()
		|| m_methodMap.count(methodName) > 0)
	{
		return ServiceMgrResult::CantAddDuplicate;
	}

	MethodID methodID = m_methods.size();
	pMethod->OnAdded(methodID);
	// Add to the map
	m_methodMap.emplace(methodName, methodID);
	m_methods.emplace_back(pMethod);
}

impl::ServiceMgrResult impl::SOServiceManager::AddService(const char* pServiceName,
	messages::QueueID serviceQueue,
	const std::shared_ptr<IService>& pService,
	SMServiceID& serviceID)
{
	lib::RWLockWrapperWrite lock{ m_mutex, m_lockState };

	auto nextServiceID = m_freeServiceID;

	lib::PathFromString objPath(pServiceName);
	lib::NodeAdder<CMServiceID> nodeAdder(true);

	lib::NodeResult addResult =
		lib::TracePath(m_serviceTree, objPath, nodeAdder);

	if (addResult != lib::NodeResult::OK)
	{
		return ServiceMgrResult::CantAddServiceName;
	}

	m_serviceTree.SetValue(nodeAdder.GetNodeID(), nextServiceID);

	++m_freeServiceID;
	m_serviceMap.emplace(std::piecewise_construct,
		std::forward_as_tuple(nextServiceID),
		std::forward_as_tuple());


}

impl::ServiceMgrResult impl::SOServiceManager::PublishService(const char* pMethodName,
	const char* pRemoteName,
	impl::SMServiceID serviceID)
{

}
impl::ServiceMgrResult impl::SOServiceManager::SendMessage(SMServiceID serviceID,
	impl::SMClientID clientID,
	const std::shared_ptr<ISOClientMessage>& pClientMessage) const
{

}
impl::ServiceMgrResult impl::SOServiceManager::DispatchMessage(SMServiceID serviceID,
	impl::SMClientID clientID,
	const std::shared_ptr<ISOServiceMessage>& pServiceMessage) const
{

}

impl::SOServiceManager& GetInstance()
{

}