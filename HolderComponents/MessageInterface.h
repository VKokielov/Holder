#pragma once

#include "AppInterface.h"
#include <cinttypes>
#include <memory>

namespace holder::message
{
	class IMessage;
	class IMessageListener;

	using ReceiverID = uint32_t;
	using MsgPtr = std::shared_ptr<IMessage>;
	using ListenerPtr = std::shared_ptr<IMessageListener>;


	class IMessage : public base::IAppObject
	{
	public:
		virtual MsgPtr GetSharedPtr() = 0;
	};

	class IMessageListener
	{
	public:
		virtual ~IMessageListener() = default;
		virtual void OnMessage(ReceiverID rcvrID,
			const MsgPtr& msgPtr) = 0;
	};

	class IMessageSendMethod : public base::IAppObject
	{
	public:
		virtual bool FindReceiver(const char* pName,
			ReceiverID& rcvrID) = 0;
		
		virtual bool SendMessage(ReceiverID rcvrID,
								 const IMessage& msg) = 0;
	};

	class IMessageReceiveMethod : public base::IAppObject
	{
	public:
		virtual ReceiverID AddListener(const char* pName,
			ListenerPtr pListener) = 0;
	};


}