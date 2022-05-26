#pragma once

#include "BaseService.h"
#include "MQDExecutor.h"
#include "ITextService.h"
#include "ExecutionManager.h"

namespace holder::test
{
	class TestServiceAlpha : public service::MQDBaseService<service::IService>,
		public std::enable_shared_from_this<TestServiceAlpha>
	{
	private:
		using ServiceBase = service::MQDBaseService<service::IService>;

		// A class that sends texts to the text service at regular intervals
		class TextSender : public base::ITimerCallback
		{
		public:
			TextSender(std::shared_ptr<stream::ITextServiceProxy> pProxy, unsigned long limit)
				:m_pTextProxy(std::move(pProxy)),
				m_limit(limit)
			{ }

			void OnTimer(base::TimerUserID timerId, base::TimerID timerID) override;
		private:
			std::shared_ptr<stream::ITextServiceProxy> m_pTextProxy;
			unsigned long m_counter{ 0 };
			unsigned long m_limit{ 0 };
		};

	public:
		TestServiceAlpha(const service::IServiceConfiguration& config);
		bool Init() override;
		void OnCreated() override;

		std::shared_ptr<MessageDequeDispatcher> GetMyMessageDispatcherSharedPtr() override;
		std::shared_ptr<base::startup::ITaskStateListener> GetMyTaskStateListenerSharedPtr() override;

	private:
		DependencyID m_didTextService;
	};
}