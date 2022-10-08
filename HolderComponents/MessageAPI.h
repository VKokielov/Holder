#pragma once

#include "MessageInterface.h"
#include "SingletonBase.h"

namespace holder::message
{
	using SendMethodID = uint64_t;
	using MessageTypeID = uint64_t;

	struct MessageDestination
	{
		SendMethodID sendMethodID;
		ReceiverID receiverID;
	};

	class MessageAPI : public base::SingletonBase
	{
	public:

		// API functions
		bool FindDestination(const char* pMethod,
			const char* pReceiver,
			MessageDestination& dest) const;

		void SendMessage(const MessageDestination& dest,
			const IMessage& message);

		void RegisterReceiver(const char* pMethod,
			const char* pReceiverName,
			const ListenerPtr& pListener);

		
	};
}