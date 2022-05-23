#pragma once

#include "Messaging.h"

#include <cinttypes>

namespace holder::service
{

	using ClientID = uint32_t;

	class IServiceLink : public base::IAppObject
	{
	public:
		virtual messages::ReceiverID GetReceiverID() const = 0;
	};

	class IServiceMessage : public messages::IMessage
	{
	public:
		virtual bool IsDestroyMessage() const = 0;
		virtual void Act(IServiceLink& object) = 0;
	};

	class IServiceObject : public base::IAppObject
	{
	public:
		virtual std::shared_ptr<IServiceLink>
			CreateProxy(const std::shared_ptr<messages::IMessageDispatcher>& pReceiver) = 0;
	};
	
}