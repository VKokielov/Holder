#pragma once

#include "BaseMessageHandler.h"
#include "IProxy.h"

namespace holder::service
{

	class ServiceLinkSendException 
	{ };

	class ServiceMessageFilter : public messages::IMessageFilter
	{
		bool CanSendMessage(const messages::IMessage& msg) override;
	};

	class DestroyClientMessage : public IServiceMessage
	{
	public:
		bool IsDestroyMessage() override;
		void Act(IServiceLink& object) override;
	};


	/*
	class BaseServiceLink : public messages::BaseMessageHandler,
		public IServiceLink
	{
	public:
		void OnMessage(const std::shared_ptr<messages::IMessage>& pMsg,
			messages::DispatchID dispatchId) override
		{
			static_cast<IServiceMessage&>(*pMsg).Act(*this);
		}

	protected:
		BaseServiceLink(const std::shared_ptr<messages::IMessageDispatcher>& pReceiveDispatcher)
			:BaseMessageHandler(std::move(pReceiveDispatcher), std::make_shared<ServiceMessageFilter>())
		{

		}

		void SetCounterpart(std::shared_ptr<messages::ISenderEndpoint> pCounterpart)
		{
			m_pCounterpart = std::move(pCounterpart);
		}



	private:
		std::shared_ptr<messages::ISenderEndpoint> m_pCounterpart;
	};

	*/

	template<typename Msg, typename ... Args>
	void SendMessage(std::shared_ptr<messages::ISenderEndpoint>& pCounterpart,
		Args&& ... args)
	{
		static_assert(std::is_base_of_v<IServiceMessage, Msg>, "Can only send classes deriving from IServiceMessage");
		auto pMsg = std::make_shared<Msg>(std::forward<Args>(args)...);

		if (!pCounterpart->SendMessage(pMsg))
		{
			throw ServiceLinkSendException();
		}
	}

}