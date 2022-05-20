#pragma once

#include "Messaging.h"

#include <memory>

namespace holder::messages
{

	// Base message handler class for a single receiver with no dispatch ID

	class BaseMessageHandler : public IMessageListener
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

		virtual void CreateReceiver();

	protected:
		virtual std::shared_ptr<BaseMessageHandler>
			GetMyBaseHandlerSharedPtr() = 0;

		std::shared_ptr<IMessageDispatcher>
			LockDispatcher() const
		{
			return m_pDispatcher.lock();
		}

		ReceiverID GetReceiverID_() const { return m_myReceiverID; }

		BaseMessageHandler(const std::shared_ptr<IMessageDispatcher>& pDispatcher,
			std::shared_ptr<IMessageFilter> pFilter)
			:m_pDispatcher(pDispatcher),
			m_pFilter(std::move(pFilter))
		{
		}

		bool HasReceiver() const { return m_hasReceiver; }

	private:
		std::weak_ptr<IMessageDispatcher> m_pDispatcher;
		std::shared_ptr<IMessageFilter> m_pFilter;

		ReceiverID m_myReceiverID;
		bool m_hasReceiver{ false };
	};


}