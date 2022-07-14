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
		enum class SpecialMessageType
		{
			None,
			CreateReceiver,
			RemoveReceiver
		};

		class MQDEnvelope
		{
		public:

			MQDEnvelope(MQDEnvelope&&) = default;
			MQDEnvelope& operator=(MQDEnvelope&&) = default;

			ReceiverID GetReceiverID() const { return m_receiverID; }

		protected:
			MQDEnvelope(ReceiverID receiverID)
				:m_receiverID(receiverID)
			{ }

		private:
			ReceiverID m_receiverID;
		};

		class MQDMessageEnvelope : public MQDEnvelope
		{
		public:
			MQDMessageEnvelope(MQDMessageEnvelope&&) = default;
			MQDMessageEnvelope& operator=(MQDMessageEnvelope&&) = default;

			MQDMessageEnvelope(ReceiverID receiverId,
				std::shared_ptr<IMessage> pMessage)
				:MQDEnvelope(receiverId),
				m_pMessage(std::move(pMessage))
			{ }

			void Act(MessageDequeDispatcher& dispatcher);
		private:
			std::shared_ptr<IMessage> m_pMessage;
		};

		class MQDCreateReceiverEnvelope : public MQDEnvelope
		{
		public:
			MQDCreateReceiverEnvelope(MQDCreateReceiverEnvelope&&) = default;
			MQDCreateReceiverEnvelope& operator=(MQDCreateReceiverEnvelope&&) = default;
			MQDCreateReceiverEnvelope(ReceiverID receiverId,
				std::shared_ptr<IMessageListener> pListener,
				std::shared_ptr<IMessageFilter> pFilter,
				DispatchID dispatchId)
				:MQDEnvelope(receiverId),
				m_pListener(std::move(pListener)),
				m_pFilter(std::move(pFilter)),
				m_dispatchId(dispatchId)
			{ }

			void Act(MessageDequeDispatcher& dispatcher);
		private:
			std::shared_ptr<IMessageListener> m_pListener;
			std::shared_ptr<IMessageFilter> m_pFilter;
			DispatchID m_dispatchId;
		};

		class MQDRemoveReceiverEnvelope : public MQDEnvelope
		{
		public:
			MQDRemoveReceiverEnvelope(MQDRemoveReceiverEnvelope&&) = default;
			MQDRemoveReceiverEnvelope& operator=(MQDRemoveReceiverEnvelope&&) = default;
			
			void Act(MessageDequeDispatcher& dispatcher);

			MQDRemoveReceiverEnvelope(ReceiverID receiverID)
				:MQDEnvelope(receiverID)
			{ }
		};


		/*
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
		*/

	public:
		ReceiverID
			CreateReceiver(const std::shared_ptr<IMessageListener>& pListener,
				std::shared_ptr<IMessageFilter> pFilter,
				DispatchID dispatchId) override;
		void RemoveReceiver(ReceiverID rcvrId) override;
		void SendMessage(ReceiverID rcvrId, std::shared_ptr<IMessage> pMessage) override;

	protected:
		// TODO: Flags if the options multiply
		MessageDequeDispatcher(bool traceLostMessages)
			:m_traceLostMessages(traceLostMessages)
		{ }

		void ProcessMessages(WorkStateDescription& workState);
		virtual void DoSignal() = 0;

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

		template<typename MsgEnvelope>
		void PostMessage(MsgEnvelope&& env, std::deque<MsgEnvelope>& queue)
		{
			std::unique_lock lkQueue(m_mutexQueue);
			queue.push_front(std::move(envelope));
			DoSignal();
		}

		std::mutex m_mutexQueue;
		// The best way is to keep three different queues for three different types of messages
		std::deque<MQDMessageEnvelope> m_queue;
		std::deque<MQDCreateReceiverEnvelope> m_createQueue;
		std::deque<MQDRemoveReceiverEnvelope> m_removeQueue;

		std::atomic<size_t> m_totalSize;

		// Local only -- no one should call CreateReceiver from any thread but the one
		// this dispatcher runs on
		std::deque <MQDMessageEnvelope> m_localQueue;
		std::deque<MQDCreateReceiverEnvelope> m_localCreateQueue;
		std::deque<MQDRemoveReceiverEnvelope> m_localRemoveQueue;

		std::atomic<ReceiverID> m_freeRcvId;
		
		std::unordered_map<ReceiverID,Receiver_>
			m_receiverMap;

		bool m_traceLostMessages{ false };
	};

}