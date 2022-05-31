#pragma once

#include "Messaging.h"

namespace holder::messages
{
	class SendException { };

	template<typename Msg, typename ... Args>
	void SendMessage(const std::shared_ptr<messages::ISenderEndpoint>& pCounterpart,
		Args&& ... args)
	{
		static_assert(std::is_base_of_v<messages::IMessage, Msg>, "Can only send classes deriving from IServiceMessage");
		auto pMsg = std::make_shared<Msg>(std::forward<Args>(args)...);

		if (!pCounterpart->SendMessage(pMsg))
		{
			throw SendException();
		}
	}
}