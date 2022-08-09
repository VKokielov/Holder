#pragma once

#include "Messaging.h"
#include "BaseMessageDispatch.h"
#include "QueueManager.h"

#include <memory>

namespace holder::messages
{

	// Base message handler class for a single receiver with no dispatch ID
	class NoDefaultReceiverException { };

	template<typename Derived, typename ... Bases>
	class BaseMessageHandler : public IMessageListener, public Bases...
	{
	protected:
		using Client = typename Derived::Client;

		class BaseClient
		{
		public:

			BaseClient(QueueID queueID, 
				messages::DispatchID dispatchID,
				std::shared_ptr<BaseMessageHandler> pListener)
			{
				CreateReceiverArgs rcvrArgs;
				rcvrArgs.dispatchId = dispatchID;
				rcvrArgs.pListener = std::move(pListener);

				m_receiverID
					= QueueManager::GetInstance().CreateReceiver(queueID,
						rcvrArgs);
			}

			~BaseClient()
			{
				QueueManager::GetInstance().RemoveReceiver(m_receiverID);
			}

			std::shared_ptr<ISenderEndpoint>
				CreateSenderEndpoint()
			{
				return
					QueueManager::GetInstance().CreateEndpoint(m_queueID, m_receiverID);
			}

		protected:
			ReceiverID GetReceiverID() const { return m_receiverID; }

		private:
			QueueID m_queueID;
			ReceiverID m_receiverID;
		};

		static_assert(std::is_base_of_v<BaseClient, Client>, "Client must derive from BaseMessageHandler::BaseClient");

	public:
		virtual void OnReceiverReady(DispatchID dispatchId) { }
		
		virtual void OnMessage(const std::shared_ptr<IMessage>& pMsg, 
			DispatchID dispatchID)
		{
			Derived* pThisDerived = static_cast<Derived*>(this);

			MessageDispatch<Derived>& dispTable = GetMessageDispatchTable();

			if (!dispTable(pThisDerived, 
				pMsg->GetTag(),
				pMsg, 
				dispatchID))
			{
				OnUnknownMessage(pMsg, dispatchID);
			}
		}

		ReceiverID GetDefaultReceiverID() const
		{
			if (!m_defaultReceiverID.has_value())
			{
				throw NoDefaultReceiverException();
			}
			return m_defaultReceiverID.value();
		}

	protected:
		virtual std::shared_ptr<BaseMessageHandler>
			GetListenerPointer() = 0;

		virtual void OnNewClient(DispatchID clientID) { }
		virtual void OnRemovingClient(DispatchID clientID) { }

		virtual void OnUnknownMessage(const std::shared_ptr<IMessage> pMsg,
			messages::DispatchID dispatchID) { }

		virtual void OnUnknownClient(const std::shared_ptr<IMessage> pMsg,
			messages::DispatchID dispatchID) { }

		BaseMessageHandler(QueueID queueID)
			:m_myQueueID(queueID)
		{

		}

		void CreateDefaultReceiver()
		{
			const std::shared_ptr<IMessageListener>& pListener = GetListenerPointer();

			CreateReceiverArgs rcvrArgs;
			rcvrArgs.dispatchId = 0;
			rcvrArgs.pListener = pListener;

			ReceiverID defaultID
				= QueueManager::GetInstance().CreateReceiver(m_myQueueID,
					rcvrArgs);

			m_defaultReceiverID.emplace(defaultID);
		}

		template<typename ... Args>
		DispatchID CreateClient(Args&.... args)
		{
			DispatchID newID{ m_freeClientID++ };

			m_clientMap.emplace(std::piecewise_construct,
				std::forward_as_tuple(newID),
				std::forward_as_tuple(m_myQueueID, newID, GetListenerPointer(),
					std::forward<Args>(args)...);

			OnNewClient(newID);
			return newID;
		}


		Client* GetClient(DispatchID dispatchID)
		{
			auto itClient = m_clientMap.find(dispatchID);

			if (itClient != m_clientMap.end())
			{
				return &itClient->second;
			}

			return nullptr;
		}

		bool RemoveClient(DispatchID dispatchID)
		{
			auto itClient = m_clientMap.find(dispatchID);
			if (itClient != m_clientMap.end())
			{
				OnRemovingClient(dispatchID);
				m_clientMap.erase(itClient);
				return true;
			}

			return false;
		}

		// For messages that can be dispatched to clients
		template<typename MsgType>
		void DispatchToClient(const std::shared_ptr<MsgType>& pMessage,
			DispatchID dispatchID)
		{
			static_cast(std::is_base_of_v<IMessage, MsgType>,
				"MsgType must derive from IMessage");

			auto itClient = m_clientMap.find(dispatchID);

			if (itClient != m_clientMap.end())
			{
				pClient = &itClient->second;
			}

			if (!pClient)
			{
				// Downcast
				OnUnknownClient(pMessage, dispatchID);
				return;
			}

			pClient->OnMessage(pMessage);
		}

		QueueID GetQueueID() const
		{
			return m_myQueueID;
		}

	private:
		
		static MessageDispatch<Derived>& GetMessageDispatchTable()
		{
			// The constructor of the dispatch calls a static function on the derived class
			// to initialize the dispatch.  Since the construction of a static variable
			// is atomic since x11, there is no data race here
			static MessageDispatch<Derived> dispatch;
			return dispatch;
		}

		QueueID m_myQueueID;
		std::optional<ReceiverID> m_defaultReceiverID;
		messages::DispatchID m_freeClientID{ 1 };
		std::unordered_map<messages::DispatchID, Client>  m_clientMap;

		bool m_hasReceiver{ false };
	};


}