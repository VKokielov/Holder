#pragma once

#include "Messaging.h"

#include <cinttypes>
#include <shared_mutex>
#include <unordered_map>

namespace holder::messages
{
	using QueueID = uint32_t;

	class SenderEndpoint : public ISenderEndpoint
	{
	public:
		bool SendMessage(const std::shared_ptr<IMessage>& pMsg) override;
	private:
		std::shared_ptr<IMessageDispatcher> m_pDispatcher;
		ReceiverID m_receiverId;
	};

	class QueueManager
	{

	public:
		QueueManager& GetInstance();

		QueueID AddQueue(const char* pQueueName, 
			std::shared_ptr<IMessageDispatcher> pQueue);

		const std::shared_ptr<IMessageDispatcher>& GetQueue(const char* pQueueName, QueueID& rQueueId);

		std::shared_ptr<ISenderEndpoint> CreateEndpoint(QueueID queueId);

	private:
		QueueManager();

		std::shared_mutex m_mutex;
	};

}