#pragma once

#include "IService.h"
#include "Messaging.h"
#include "IProxy.h"
#include "ServiceMessageLib.h"
#include "BaseMessageHandler.h"
#include "MessageTypeTags.h"
#include "TypeTagDisp.h"
#include "QueueManager.h"

namespace holder::service
{

	template<typename Derived, typename ...Bases>
	class BaseProxy : public messages::BaseMessageHandler<Derived,
		Bases...>,
		public IServiceLink
	{
	private:
		using BaseType = messages::BaseMessageHandler<Derived,
			Bases...>;

	public:
		BaseProxy(messages::QueueID myQueueID)
			:BaseType(myQueueID)
		{

		}

		~BaseProxy()
		{
			if (m_pRemote)
			{
				auto pDestroyMessage = std::make_shared<DestroyClientMessage>();
				SendMessage(pDestroyMessage);
			}
		}

		void Initialize()
		{
			BaseType::CreateDefaultReceiver();
		}

		void SetRemote(messages::QueueID queueID, messages::ReceiverID receiverID)
		{
			auto& queueManager = messages::QueueManager::GetInstance();

			m_pRemote = queueManager.CreateEndpoint(queueID, receiverID);
		}

	protected:

		template<typename Msg>
		void SendMessage(const std::shared_ptr<Msg>& pMessage)
		{
			m_pRemote->SendMessage(pMessage);
		}

		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			                              TagDispatch& tagDispatchTable)
		{ 
			tagDispatchTable.AddDispatch(base::constants::GetServiceMessageTag(),
				&BaseProxy::OnServiceMessage);
		}

	private:

		void OnServiceMessage(const std::shared_ptr<messages::IMessage>& pMessage,
			messages::DispatchID dispatchID)
		{
			static_cast<IServiceMessage*>(pMessage.get())->Act(*this);
		}

		std::shared_ptr<messages::ISenderEndpoint> m_pRemote;
	};

	


}