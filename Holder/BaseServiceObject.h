#pragma once

#include "BaseMessageHandler.h"
#include "BaseServiceLink.h"
#include "BaseClientObject.h"
#include "BaseProxy.h"
#include "IProxy.h"

#include <vector>
#include <type_traits>
#include <shared_mutex>
#include <memory>

namespace holder::service
{

	template<typename D, typename Proxy, typename Client>
	class BaseServiceObject : public messages::IMessageListener
	{
	private:
		using ObjType = BaseServiceObject<D, Proxy, Client>;
		static_assert(std::is_base_of_v<BaseProxy, Proxy>, "Proxy must derive from BaseProxy");
		static_assert(std::is_base_of_v<BaseClientObject, Client>, "Client must derive from BaseClientObject");
	public:
		void OnMessage(const std::shared_ptr<messages::IMessage>& pMsg,
			messages::DispatchID dispatchID) override
		{
			Client* pClient{ nullptr };

			{
				std::shared_lock lk{ m_mutex };

				auto itClient = m_clientMap.find(dispatchID);

				if (itClient != m_clientMap.end())
				{
					pClient = &itClient->second;
				}
			}

			if (!pClient)
			{
				return;
			}

			auto pServiceMessage = static_cast<IServiceMessage*>(pMsg.get());

			if (pServiceMessage->IsDestroyMessage())
			{
				// Remove the client
				RemoveClient(dispatchID);
				return;
			}

			// Otherwise simply act
			pServiceMessage->Act(*pClient);
		}

	protected:
		BaseServiceObject()
		{ }

		virtual std::shared_ptr<IMessageListener>
			GetMyListenerSharedPtr() = 0;

		void SetDispatcher(const std::shared_ptr<messages::IMessageDispatcher>& pLocalDispatcher)
		{
			m_pLocalDispatcher = pLocalDispatcher;
		}

		std::shared_ptr<IServiceLink>
			CreateProxy_(const std::shared_ptr<messages::IMessageDispatcher>& pRemoteDispatcher)
		{
			std::unique_lock lk{ m_mutex };

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
					std::make_shared<ServiceMessageFilter>(),
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

			return pServiceLink;
		}
	private:

		void RemoveClient(messages::DispatchID clientID)
		{
			// Remove a client from the dispatcher and by removing it from the map
			std::unique_lock lk{ m_mutex };

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

		}

		std::shared_mutex m_mutex;
		std::unordered_map<messages::DispatchID, Client> m_clientMap;
		messages::DispatchID m_nextClientID{ 0 };

		// Local dispatcher
		std::weak_ptr<messages::IMessageDispatcher>  m_pLocalDispatcher;
	};



}