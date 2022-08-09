#include "MQDExecutor.h"
#include "DTUtils.h"
#include "IAppObjectFactory.h"

namespace impl_ns = holder::messages;

impl_ns::MQDExecutor::MQDExecutor(const char* pthreadName, bool traceLostMessages)
	:MessageDequeDispatcher(traceLostMessages),
	m_threadName(pthreadName)
{ }

void impl_ns::MQDExecutor::InitExecutor()
{
	if (m_myExecutor.load() == base::EXEC_WILDCARD)
	{
		auto pMyBase = GetExecutorSharedPtr();

		m_myExecutor.store(base::ExecutionManager::GetInstance().AddExecutor(m_threadName.c_str(),
			pMyBase));
	}
}

holder::base::ExecutionState impl_ns::MQDExecutor::Run(holder::base::ExecutionArgs& args)
{
	WorkStateDescription queueWork{ 0,0 };
	MessageDequeDispatcher::ProcessMessages(queueWork);

	if (m_endRequested)
	{
		// There may be messages in the queue, but there's nothing to do about it effectively
		return base::ExecutionState::End;
	}

	if (queueWork.msgsRemaining > 0)
	{
		return base::ExecutionState::Continue;
	}

	return base::ExecutionState::Suspend;
}

void impl_ns::MQDExecutor::TerminationRequested()
{
	m_endRequested = true;
}

void impl_ns::MQDExecutor::DoSignal()
{
	base::ExecutionManager::GetInstance().SignalExecutor(m_myExecutor.load(), base::ExecutorSignalType::WakeUp);
}

bool impl_ns::MQDExecutor::Init()
{
	return true;
}
void impl_ns::MQDExecutor::DeInit()
{

}

// DefaultMQDExecutor
std::tuple<std::string, bool> impl_ns::DefaultMQDExecutor::Unpacker::Unpack(const data::IDatum& datum)
{
	std::string threadName;
	if (data::GetDictValue<std::string>(datum, "threadName", threadName)
		!= data::AccessResult::OK)
	{
		throw holder::base::UnpackArgumentsException();
	}

	bool traceLostMessages;
	if (data::GetDictValue<bool>(datum, "traceLostMessages", traceLostMessages)
		!= data::AccessResult::OK)
	{
		throw holder::base::UnpackArgumentsException();
	}

	return std::make_tuple(threadName, traceLostMessages);
}
