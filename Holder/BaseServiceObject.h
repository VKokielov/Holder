#pragma once

#include "BaseMessageHandler.h"
#include "ServiceMessageLib.h"
#include "BaseProxy.h"
#include "IProxy.h"
#include "TypeTagDisp.h"
#include "MessageTypeTags.h"
#include "ExecutionManager.h"
#include "QueueManager.h"

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
		CreateProxyMessage(messages::QueueID queueID)
			:m_queueID(queueID)
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

			auto pProxy = obj.CreateProxyLocal(m_queueID);

			m_serviceLinkPromise.set_value(pProxy);
		}
		
		std::future< std::shared_ptr<IServiceLink> >
			GetFuture()
		{
			return m_serviceLinkPromise.get_future();
		}

	private:
		messages::QueueID m_queueID;
		std::promise<std::shared_ptr<IServiceLink> >
			m_serviceLinkPromise;
	};

	template<typename Derived, typename ... Bases>
	class BaseServiceObject : public messages::BaseMessageHandler<Derived,
		Bases...>,
		public IServiceObject
	{
	private:
		using Client = typename Derived::Client;
		using Proxy = typename Derived::Proxy;

		using BaseType = messages::BaseMessageHandler<Derived,
			Bases...>;

		using ObjType = BaseServiceObject<D, Proxy, Client>;
		static_assert(std::is_base_of_v<BaseProxy, Proxy>, "Proxy must derive from BaseProxy");

		class ServiceObjClient : 
			public BaseType::BaseClient,
			public IServiceLink
		{
		public:

			ServiceObjClient(messages::QueueID localQueueID,
				messages::DispatchID localDispatchID,
				std::shared_ptr<BaseType> pListener,
				messages::QueueID remoteQueueID,
				messages::ReceiverID remoteReceiverID)
				:BaseType::BaseClient(localQueueID, localDispatchID, pListener),
				m_remoteQueueID(remoteQueueID),
				m_remoteReceiverID(remoteReceiverID)
			{
				auto& queueManager = messages::QueueManager::GetInstance();

				m_pEndpoint = queueManager.CreateEndpoint(remoteQueueID, remoteReceiverID);
			}

			void OnMessage(const std::shared_ptr<IServiceMessage>& pServiceMessage)
			{
				pServiceMessage->Act(*this);
			}
			messages::ReceiverID GetReceiverID() const
			{
				return BaseType::BaseClient::GetReceiverID();
			}

			template<typename Msg>
			void SendMessage(const std::shared_ptr<Msg>& pMessage)
			{
				m_pEndpoint->SendMessage(pMessage);
			}

		private:
			std::shared_ptr<messages::ISenderEndpoint> m_pEndpoint;
			messages::QueueID m_remoteQueueID;
			messages::ReceiverID m_remoteReceiverID;
		};

	public:
		void OnServiceMessage(const std::shared_ptr<messages::IMessage>& pMsg,
			messages::DispatchID dispatchID)
		{
			// TODO:  Is there a way to do this without constructing a new shared_ptr?
			auto pServiceMessage
				= std::static_pointer_cast<IServiceMessage>(pMsg);

			BaseType::DispatchToClient<IServiceMessage>(pServiceMessage, dispatchID);
		}

		void OnDestroyMessage(const std::shared_ptr<messages::IMessage>& pMsg,
			messages::DispatchID dispatchID)
		{
			BaseType::RemoveClient(dispatchID);
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

		std::shared_ptr<IServiceLink>
			CreateProxy(messages::QueueID remoteQueueID) override
		{
			// NOTE:  This function acts differently depending on where we are operating
			// From within the service's thread, it simply calls GetProxyLocal()

			auto& queueManager = messages::QueueManager::GetInstance();

			if (queueManager.IsOnSameThread(remoteQueueID,
				BaseType::GetQueueID()))
			{
				return CreateProxyLocal(remoteQueueID);
			}

			// Otherwise it uses a promise and a future to execute this same procedure
			// on the remote thread of the service
			auto pCreateProxyMessage =
				std::make_shared<CreateProxyMessage>(remoteQueueID);

			auto proxyFuture = pCreateProxyMessage->GetFuture();

			queueManager.SendMessage(BaseType::GetQueueID(), 
				BaseType::GetDefaultReceiverID());

			proxyFuture.wait();

			return proxyFuture.get();
		}

	protected:
		BaseServiceObject(messages::QueueID localQueueID);

	private:
		std::shared_ptr<IServiceLink>
			CreateProxyLocal(messages::QueueID remoteQueueID)
		{
			std::shared_ptr<Proxy> pProxy;
			auto& queueManager = messages::QueueManager::GetInstance();

			pProxy = std::make_shared<Proxy>(remoteQueueID);
			pProxy->Initialize();

			messages::ReceiverID remoteReceiverID = pProxy->GetDefaultReceiverID();

			// Create a client and a corresponding proxy
			auto clientID = BaseType::CreateClient(remoteQueueID, remoteReceiverID);
			ServiceObjClient* pCreatedClient = BaseType::GetClient(clientID);

			pProxy->SetRemote(BaseType::GetQueueID(), 
				pCreatedClient->GetReceiverID());

			return pProxy;
		}
		
		friend class CreateProxyMessage;

		messages::QueueID m_queueID;
	};



}