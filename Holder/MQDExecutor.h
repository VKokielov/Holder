#pragma once

#include <string>

#include "MessageDequeDispatcher.h"
#include "ExecutionManager.h"

namespace holder::messages
{
	// A dispatcher that implements itself as an executor
	// When the executor thread begins executing, the derived class gets an OnReady() callback,
	// after which it can create a receiver for its messages

	class MQDExecutor : public MessageDequeDispatcher,
		public base::IExecutor
	{
	public:
		base::ExecutionState Run(base::ExecutionArgs& args) override;
		void TerminationRequested() override;
		bool Init() override;
		void DeInit() override;
	protected:
		MQDExecutor(const char* pThreadName, bool traceLostMessages);
		void InitExecutor();
		void DoSignal() override;


	private:
		std::string m_threadName;
		base::ExecutorID m_myExecutor{ base::EXEC_WILDCARD };
		bool m_endRequested{ false };
	};

}