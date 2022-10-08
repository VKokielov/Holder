#pragma once

#include "MessageInterface.h"
#include "MessageAPI.h"

namespace holder::message
{

	class MessageManager : public MessageAPI
	{
	public:

		// Manager functions
		void AddReceiveMethod(const char* pName,
			std::shared_ptr<IMessageReceiveMethod> pReceiveMethod);

		void AddSendMethod(const char* pName,
			std::shared_ptr<IMessageSendMethod> pSendMethod);

	};


}