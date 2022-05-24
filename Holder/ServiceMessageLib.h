#pragma once

#include "BaseMessageHandler.h"
#include "IProxy.h"
#include "MessageTypeTags.h"

namespace holder::service
{

	class ServiceLinkSendException 
	{ };

	class ServiceMessageFilter : public messages::IMessageFilter
	{
		bool CanSendMessage(const messages::IMessage& msg) override;
	};

	class DestroyClientMessage : public messages::IMessage
	{
	public:
		base::types::TypeTag GetTag() const override;
	};

	template<typename Derived,
		     typename Client>
	class TypedServiceMessage : public IServiceMessage
	{
	public:
		base::types::TypeTag GetTag() const override
		{
			return base::constants::GetServiceMessageTag();
		}

		void Act(IServiceLink& object) override
		{
			static_cast<Derived*>(this)->TypedAct(static_cast<Client&>(object));
		}
	};

	template<typename Msg, typename ... Args>
	void SendMessage(const std::shared_ptr<messages::ISenderEndpoint>& pCounterpart,
		Args&& ... args)
	{
		static_assert(std::is_base_of_v<messages::IMessage, Msg>, "Can only send classes deriving from IServiceMessage");
		auto pMsg = std::make_shared<Msg>(std::forward<Args>(args)...);

		if (!pCounterpart->SendMessage(pMsg))
		{
			throw ServiceLinkSendException();
		}
	}

}