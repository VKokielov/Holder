#pragma once

#include "MessageDequeDispatcher.h"
#include "ExecutionManager.h"
#include "IDataTree.h"

#include <atomic>
#include <string>


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
		base::ExecutorID GetExecutorID() const { return m_myExecutor.load(); }

		const char* GetExecutionThreadName() const override { return m_threadName.c_str(); }

		virtual const std::shared_ptr<IExecutor>&
			GetExecutorSharedPtr() = 0;

	private:
		std::string m_threadName;
		std::atomic<base::ExecutorID> m_myExecutor{ base::EXEC_WILDCARD };
		bool m_endRequested{ false };
	};

	class DefaultMQDExecutor : public MQDExecutor,
		public std::enable_shared_from_this<DefaultMQDExecutor>
	{
	public:
		struct Unpacker
		{
			static std::tuple<std::string, bool> Unpack(const data::IDatum& datum);
		};

		DefaultMQDExecutor(const std::string& strName, bool traceLostMessages);

		const std::shared_ptr<IExecutor>&
			GetExecutorSharedPtr() override;
	};

}