#pragma once

#include "IAppObject.h"
#include "TypeTags.h"

#include <cinttypes>
#include <memory>

namespace holder::messages
{

	using DispatchID = uint32_t;
	using ReceiverID = uint32_t;

	class MessageException { };

	class IMessage : public base::IAppObject
	{
	public:
		virtual base::types::TypeTag GetTag() const = 0;
	};

	class ISenderEndpoint : public base::IAppObject
	{
	public:
		// TODO:  Support sending batches
		virtual bool SendMessage(const std::shared_ptr<IMessage>& pMsg) = 0;
	};

	class IMessageListener : public base::IAppObject
	{
	public:
		virtual void OnReceiverReady(DispatchID dispatchId) = 0;
		virtual void OnMessage(const std::shared_ptr<IMessage>& pMsg, DispatchID dispatchId) = 0;
	};


	// A filter runs sender-side and verifies that messages have the right type before they are
	// sent.  This ensures that they are not simply lost.
	class IMessageFilter : public base::IAppObject
	{
	public:
		virtual bool CanSendMessage(const IMessage& msg) = 0;
	};

	class IMessageDispatcher : public base::IAppObject
	{
	public:
		virtual ReceiverID
			CreateReceiver(const std::shared_ptr<IMessageListener>& pListener,
				std::shared_ptr<IMessageFilter> pFilter,
				DispatchID dispatchId) = 0;
		virtual void RemoveReceiver(ReceiverID rcvrId) = 0;
		virtual void SendMessage(ReceiverID rcvrId, std::shared_ptr<IMessage> pMessage)
			= 0;
		virtual const char* GetExecutionThreadName() const = 0;
	};

}