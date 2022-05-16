#pragma once

#include "Messaging.h"
#include <memory>
#include <shared_mutex>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

namespace holder::messages
{

	// A deque-based message dispatcher, with customizable signal and block calls
	// Since blocking/signal behavior may be different in thread vs task-based environments,
	// this abstraction makes sense
	class MessageDequeDispatcher : public IMessageDispatcher
	{
	protected:

		struct WorkStateDescription
		{
			size_t msgsToProcess;
			size_t msgsRemaining;
		};

	private:
		class MQDEnvelope
		{
		public:
			MQDEnvelope(std::shared_ptr<IMessage> pMessage,
				ReceiverID receiverID)
				:m_receiverID(receiverID)
			{ }

			MQDEnvelope(MQDEnvelope&&) = default;
			MQDEnvelope& operator=(MQDEnvelope&&) = default;

			const std::shared_ptr<IMessage>& GetMessage() const
			{
				return m_pMessage;
			}

			ReceiverID GetReceiverID() const { return m_receiverID; }

		private:
			std::shared_ptr<IMessage> m_pMessage;
			ReceiverID m_receiverID;
		};

		class MQDSenderEndpoint : public ISenderEndpoint
		{
		public:
			MQDSenderEndpoint(std::shared_ptr<IMessageFilter> pFilter,
				std::shared_ptr<MessageDequeDispatcher> pDispatcher,				
				ReceiverID receiverID)
				:m_pFilter(pFilter),
				m_pDispatcher(pDispatcher),
				m_receiverID(receiverID)
			{ }

			bool SendMessage(const std::shared_ptr<IMessage>& pMsg) override;

		private:
			std::shared_ptr<IMessageFilter> m_pFilter;
			std::weak_ptr<MessageDequeDispatcher> m_pDispatcher;
			ReceiverID m_receiverID;
		};

	public:
		ReceiverID
			CreateReceiver(const std::shared_ptr<IMessageListener>& pListener,
				std::shared_ptr<IMessageFilter> pFilter,
				DispatchID dispatchId) override;
		bool RemoveReceiver(ReceiverID rcvrId) override;
		virtual std::shared_ptr<ISenderEndpoint>
			CreateEndpoint(ReceiverID rcvrId) override;

	protected:
		// TODO: Flags if the options multiply
		MessageDequeDispatcher(bool traceLostMessages)
			:m_traceLostMessages(traceLostMessages)
		{ }

		void ProcessMessages(WorkStateDescription& workState);
		virtual void DoSignal() = 0;
		virtual std::shared_ptr<MessageDequeDispatcher> GetMyMessageDispatcherSharedPtr() = 0;

		virtual void OnLostMessage(const std::shared_ptr<IMessage>& pMessage,
			ReceiverID rcvId);

	private:
		struct Receiver_
		{
			ReceiverID id{ 0 };
			DispatchID dispatchId{ 0 };
			std::shared_ptr<IMessageListener> pListener;
			std::shared_ptr<IMessageFilter> pFilter;

			Receiver_(ReceiverID id_, 
				DispatchID dispatchId_,
				std::shared_ptr<IMessageListener> pListener_,
				std::shared_ptr<IMessageFilter> pFilter_)
				:id(id_),
				dispatchId(dispatchId_),
				pListener(pListener_),
				pFilter(pFilter_)
			{ }

			void Dispatch(const std::shared_ptr<IMessage>& pMsg)
			{
				pListener->OnMessage(pMsg, dispatchId);
			}
		};
		
		void SendMessage(MQDEnvelope&& env);

		std::mutex m_mutexQueue;
		std::deque<MQDEnvelope> m_queue;
		std::atomic<size_t> m_totalSize;

		// Local only -- no one should call CreateReceiver from any thread but the one
		// this dispatcher runs on
		std::deque<MQDEnvelope> m_localQueue;
		
		std::unordered_map<ReceiverID,Receiver_>
			m_receiverMap;
		ReceiverID m_freeRcvId{ 0 };

		bool m_traceLostMessages{ false };
	};

}