#pragma once

#include "BaseMessageHandler.h"
#include "ServiceMessageLib.h"
#include "BaseClientObject.h"
#include "BaseProxy.h"
#include "IProxy.h"
#include "TypeTagDisp.h"
#include "MessageTypeTags.h"
#include "ExecutionManager.h"

#include <vector>
#include <type_traits>
#include <shared_mutex>
#include <memory>
#include <future>

namespace holder::service
{

	class CreateProxyMessage : public messages::IMessage
	{
	public:
		CreateProxyMessage(std::shared_ptr<messages::IMessageDispatcher> pDispatcher)
			:m_pDispatcher(pDispatcher)
		{ }

		base::types::TypeTag GetTag() const override
		{
			return base::constants::GetCreateProxyMessageTag();
		}

		template<typename ServiceObject>
		void Act(ServiceObject& obj)
		{
			// This function invokes the CreateProxyLocal function on the service object
			// in question and stores the result into the associated Promise

			auto pProxy = obj.CreateProxyLocal(std::move(m_pDispatcher));

			m_serviceLinkPromise.set_value(pProxy);
		}
		
		std::future< std::shared_ptr<IServiceLink> >
			GetFuture()
		{
			return m_serviceLinkPromise.get_future();
		}
	private:
		std::shared_ptr< messages::IMessageDispatcher> m_pDispatcher;
		std::promise<std::shared_ptr<IServiceLink> >
			m_serviceLinkPromise;
	};

	template<typename D, typename Proxy, typename Client>
	class BaseServiceObject : public messages::IMessageListener
	{
	private:
		using ObjType = BaseServiceObject<D, Proxy, Client>;
		static_assert(std::is_base_of_v<BaseProxy, Proxy>, "Proxy must derive from BaseProxy");
		static_assert(std::is_base_of_v<BaseClientObject, Client>, "Client must derive from BaseClientObject");
	public:
		void OnServiceMessage(messages::IMessage& rMsg,
			messages::DispatchID dispatchID)
		{
			Client* pClient{ nullptr };

			auto itClient = m_clientMap.find(dispatchID);

			if (itClient != m_clientMap.end())
			{
				pClient = &itClient->second;
			}

			if (!pClient)
			{
				return;
			}

			auto& rServiceMessage = static_cast<IServiceMessage&>(rMsg);

			rServiceMessage.Act(*pClient);
		}

		void OnDestroyMessage(messages::IMessage& rMsg,
			messages::DispatchID dispatchID)
		{
			RemoveClient(dispatchID);
		}

		void OnCreateProxyMessage(messages::IMessage& rMsg,
			messages::DispatchID dispatchID)
		{
			static_cast<CreateProxyMessage&>(rMsg).Act(*this);
		}

		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			TagDispatch& tagDispatchTable)
		{
			tagDispatchTable.AddDispatch(base::constants::GetServiceMessageTag(),
				&BaseServiceObject::OnServiceMessage);
			tagDispatchTable.AddDispatch(base::constants::GetDestroyMessageTag(),
				&BaseServiceObject::OnDestroyMessage);
			tagDispatchTable.AddDispatch(base::constants::GetCreateProxyMessageTag(),
				&BaseServiceObject::OnCreateProxyMessage);
		}

	protected:
		BaseServiceObject()
		{ }

		void SetThreadName(const char* pThreadName)
		{
			m_threadName = pThreadName;
		}

		virtual std::shared_ptr<IMessageListener>
			GetMyListenerSharedPtr() = 0;

		// Client created callback
		virtual void OnCreateClient(messages::DispatchID clientID,
									messages::ReceiverID clientReceiverID,
									const std::shared_ptr<messages::ISenderEndpoint>&
										pRemoteEndpoint)
		{ }

		// Client destroyed callback
		virtual void OnDestroyClient(messages::DispatchID clientID)
		{ }

		void SetDispatcher(const std::shared_ptr<messages::IMessageDispatcher>& pLocalDispatcher)
		{
			// Create a default receiver and endpoint
			m_defaultReceiverID = pLocalDispatcher->CreateReceiver(GetMyListenerSharedPtr(),
				std::shared_ptr<messages::IMessageFilter>(),
				0);

			m_pDefaultEndpoint = pLocalDispatcher->CreateEndpoint(m_defaultReceiverID);
			m_pLocalDispatcher = pLocalDispatcher;
		}

		std::shared_ptr<IServiceLink>
			CreateProxy_(const std::shared_ptr<messages::IMessageDispatcher>& pRemoteDispatcher)
		{
			// NOTE:  This function acts differently depending on where we are operating
			// From within the service's thread, it simply calls GetProxyLocal()
			if (base::ExecutionManager::GetInstance().GetCurrentThreadName()
				== m_threadName)
			{
				return CreateProxyLocal(pRemoteDispatcher);
			}


			if (!m_pDefaultEndpoint)
			{
				return std::shared_ptr<IServiceLink>();
			}
			
			// Otherwise it uses a promise and a future to execute this same procedure
			// on the remote thread of the service
			auto pCreateProxyMessage =
				std::make_shared<CreateProxyMessage>(pRemoteDispatcher);

			auto proxyFuture = pCreateProxyMessage->GetFuture();

			m_pDefaultEndpoint->SendMessage(pCreateProxyMessage);

			proxyFuture.wait();

			return proxyFuture.get();
		}
	private:

		std::shared_ptr<IServiceLink>
			CreateProxyLocal(const std::shared_ptr<messages::IMessageDispatcher>& pRemoteDispatcher)
		{
			auto pLocalDispatcher = m_pLocalDispatcher.lock();
			std::shared_ptr<IServiceLink> pServiceLink;
			std::shared_ptr<Proxy> pProxy;

			if (!pLocalDispatcher)
			{
				// Where is the dispatcher?  Can't create proxy
				return pServiceLink;
			}

			// We now create a new client object and a proxy to go with it.
			// We need a sender endpoint for each, but we can only create these after we know the 
			// receiver ID

			messages::DispatchID clientID = m_nextClientID++;

			// First create a receiver for this client object
			messages::ReceiverID receiverID
				= pLocalDispatcher->CreateReceiver(GetMyListenerSharedPtr(),
					std::shared_ptr<messages::IMessageFilter>(),
					clientID);

			auto pClientEndpoint = pLocalDispatcher->CreateEndpoint(receiverID);
			// Create a proxy (which will create its own receiver)
			pProxy = std::make_shared<Proxy>(pRemoteDispatcher, pClientEndpoint);
			pProxy->CreateReceiver();

			pServiceLink = pProxy;

			// Create the remote sender endpoint for the proxy
			auto pProxyEndpoint = pProxy->CreateSenderEndpoint();

			// Now create the object
			auto emplResult = m_clientMap.emplace(std::piecewise_construct,
				std::forward_as_tuple(clientID),
				std::forward_as_tuple(static_cast<D&>(*this),
					clientID,
					receiverID,
					pProxyEndpoint));

			OnCreateClient(clientID, receiverID, pProxyEndpoint);

			return pServiceLink;
		}
		void RemoveClient(messages::DispatchID clientID)
		{
			// Remove a client from the dispatcher and by removing it from the map

			auto itClient = m_clientMap.find(clientID);

			if (itClient == m_clientMap.end())
			{
				return;
			}

			Client& rClient = itClient->second;

			auto pLocalDispatcher = m_pLocalDispatcher.lock();

			if (pLocalDispatcher)
			{
				// Remove the client's receiver
				pLocalDispatcher->RemoveReceiver(rClient.GetReceiverID());
			}

			// Remove the client
			m_clientMap.erase(itClient);

			// Callback
			OnDestroyClient(clientID);
		}

		friend class CreateProxyMessage;

		std::string m_threadName;

		std::unordered_map<messages::DispatchID, Client> m_clientMap;
		messages::DispatchID m_nextClientID{ 0 };

		// Local dispatcher
		std::weak_ptr<messages::IMessageDispatcher>  m_pLocalDispatcher;
		// Default receiver and endpoint
		messages::ReceiverID m_defaultReceiverID{};
		std::shared_ptr<messages::ISenderEndpoint> m_pDefaultEndpoint;
	};



}