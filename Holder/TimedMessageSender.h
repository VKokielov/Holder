#pragma once

#include "IExecutor.h"
#include "Messaging.h"

#include <tuple>

namespace holder::messages
{

	template<typename Msg, typename ... Args>
	class TimedMessageSender : public base::ITimerCallback
	{
	public:

		void OnTimer(base::TimerID timerUserId, base::TimerID timerId) override
		{
			auto pMsg = std::apply(std::make_shared<Msg>, GetConstructorArgs());
			m_pEndpoint->SendMessage(pMsg);
		}

	protected:
		virtual std::tuple<Args...> GetConstructorArgs() = 0;

		TimedMessageSender(std::shared_ptr<ISenderEndpoint> pEndpoint)
			:m_pEndpoint(pEndpoint)
		{ }

	private:
		std::shared_ptr<ISenderEndpoint>  m_pEndpoint;
	};

	// Specialization for no arguments
	template<typename Msg>
	class TimedMessageSender<Msg> : public base::ITimerCallback
	{
	public:
		TimedMessageSender(std::shared_ptr<ISenderEndpoint> pEndpoint)
			:m_pEndpoint(pEndpoint)
		{ }

		void OnTimer(base::TimerID timerId) override
		{
			auto pMsg = std::make_shared<Msg>();
			m_pEndpoint->SendMessage(pMsg);
		}

	private:
		std::shared_ptr<ISenderEndpoint>  m_pEndpoint;
	};


}