#pragma once

#include "Messaging.h"

#include <memory>

namespace holder::messages
{

	// Base message handler class for a single receiver with no dispatch ID

	class BaseMessageHandler : public IMessageListener,
		public std::enable_shared_from_this<BaseMessageHandler>
	{
	public:
		~BaseMessageHandler()
		{
			auto pDispatcher = m_pDispatcher.lock();

			if (pDispatcher)
			{
				pDispatcher->RemoveReceiver(m_myReceiverID);
			}
		}

		std::shared_ptr<messages::ISenderEndpoint>
			CreateSenderEndpoint();
	protected:

		std::shared_ptr<IMessageDispatcher>
			LockDispatcher() const
		{
			return m_pDispatcher.lock();
		}

		ReceiverID GetReceiverID_() const { return m_myReceiverID; }

		BaseMessageHandler(const std::shared_ptr<IMessageDispatcher>& pDispatcher,
			std::shared_ptr<IMessageFilter> pFilter)
			:m_pDispatcher(pDispatcher)
		{
			m_myReceiverID = pDispatcher->CreateReceiver(shared_from_this(), pFilter, 0);
		}

	private:
		std::weak_ptr<IMessageDispatcher> m_pDispatcher;
		ReceiverID m_myReceiverID;
	};


}