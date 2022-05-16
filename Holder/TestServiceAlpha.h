#pragma once

#include "BaseService.h"
#include "MQDExecutor.h"
#include "ITextService.h"
#include "ExecutionManager.h"

namespace holder::test
{
	class TestServiceAlpha : public service::MQDBaseService<service::IService>
	{
	private:
		using ServiceBase = service::MQDBaseService<service::IService>;

		// A class that sends texts to the text service at regular intervals
		class TextSender : public base::ITimerCallback
		{
		public:
			TextSender(std::shared_ptr<stream::ITextServiceProxy> pProxy)
				:m_pTextProxy(std::move(pProxy))
			{ }

			void OnTimer(base::TimerID timerId) override;
		private:
			std::shared_ptr<stream::ITextServiceProxy> m_pTextProxy;
			unsigned long m_counter{ 0 };
		};

	public:
		TestServiceAlpha(const service::IServiceConfiguration& config);
		bool Init() override;

	private:
		DependencyID m_didTextService;
	};
}