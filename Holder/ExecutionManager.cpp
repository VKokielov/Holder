#include "ExecutionManager.h"
#include "SingletonConfig.h"

#include <iostream>

namespace impl_ns = holder::base;

void impl_ns::ExecutionManager::ExecutorThread::operator()(
	ExecutorID firstExecId,
	std::shared_ptr<IExecutor> pFirstExecutor)
{
	// Add the initial executor
	AddExecutor(firstExecId, pFirstExecutor);

	// Begin the loop
	bool shouldRun{ true };
	bool justWaited{ false };

	std::vector<ExecutionInstructionMessage> localInstructionList;

	while (shouldRun)
	{
		bool expNI{ true };
		if (justWaited || m_newInstructions.compare_exchange_strong(expNI, false))
		{
			justWaited = false;
			{
				std::unique_lock lk{ m_mutexInstructions };
				localInstructionList.clear();
				std::swap(m_instructionList, localInstructionList);
			}

			// First process adds, then signals.  This ensures that a "send to all" signal
			// really is sent to all, so there's no race condition between an executor being added
			// and a termination signal
			for (const ExecutionInstructionMessage& msg : localInstructionList)
			{
				if (msg.instructionTag == ExecutionInstructionTag::AddExecutor)
				{
					AddExecutor(msg.execId, msg.executor);
				}
			}

			for (const ExecutionInstructionMessage& msg : localInstructionList)
			{
				if (msg.instructionTag == ExecutionInstructionTag::SignalExecutor)
				{
					HandleExecutorSignal(msg.execId, msg.signalType);
				}
			}
		}

		// Now go through the active executors round-robin and call them
		std::vector<ExecutorID> execsToRemove;
		for (ExecutorState& execState : m_executors)
		{
			if (execState.stateTag == ExecutorStateTag::Executing)
			{
				ExecutionArgs execArgs;
				ExecutionState nextState = execState.pExecutor->Run(execArgs);

				if (nextState == ExecutionState::Suspend)
				{
					// Set the tag to "suspended" and decrease the number of active
					// executors
					execState.stateTag = ExecutorStateTag::Suspended;
					--m_nActive;
				}
				else if (nextState == ExecutionState::End)
				{
					execsToRemove.emplace_back(execState.execId);
				}
			}
		}

		// Remove any executors that asked to be removed
		if (!execsToRemove.empty())
		{
			for (ExecutorID idToRemove : execsToRemove)
			{
				RemoveExecutor(idToRemove);
			}
			ExecutionManager::GetInstance().RemoveExecutors(execsToRemove);
		}

		// If the number of active executors is 0, go to sleep
		if (m_nActive == 0)
		{
			// Wait / sleep with a periodic wakeup
			std::unique_lock lk{ m_mutexInstructions };
			shouldRun = !m_instructionList.empty() || !m_executors.empty() || !m_isDoomed;
			
			// shouldRun false means the thread is doomed, has no executors and its instruction list is empty
			// in effect, 
			if (shouldRun)
			{
				m_cvInstructions.wait_for(lk, ExecutionManager::GetInstance().GetIdleTimeout());
				shouldRun = !m_instructionList.empty() || !m_executors.empty() || !m_isDoomed;
			}

			justWaited = true;
		}
		
	}

	// At this point there should be NO pending executors.

	// Remove myself
	ExecutionManager::GetInstance().RemoveThread(m_threadId);
}

void impl_ns::ExecutionManager::ExecutorThread::AddExecutor(ExecutorID execId,
	std::shared_ptr<IExecutor> pExecutor)
{
	size_t localId = m_executors.size();
	m_executors.emplace_back(execId, ExecutorStateTag::Executing, pExecutor);
	m_signalMap.emplace(execId, localId);
	++m_nActive;

	pExecutor->Init();
}

void impl_ns::ExecutionManager::ExecutorThread::HandleSignalForIndex(size_t execIndex, ExecutorSignalType signalType)
{
	auto& rExec = m_executors[execIndex];

	if (signalType == ExecutorSignalType::RequestTermination)
	{
		rExec.pExecutor->TerminationRequested();
	}
	// Every signal is also a wakeup
	if (rExec.stateTag == ExecutorStateTag::Suspended)
	{
		++m_nActive;
		rExec.stateTag = ExecutorStateTag::Executing;
	}
}

void impl_ns::ExecutionManager::ExecutorThread::HandleExecutorSignal(ExecutorID execId, ExecutorSignalType signalType)
{
	if (execId != EXEC_WILDCARD)
	{
		auto itExec = m_signalMap.find(execId);

		if (itExec != m_signalMap.end())
		{
			HandleSignalForIndex(itExec->second, signalType);
		}
	}
	else
	{
		for (size_t i = 0; i < m_executors.size(); ++i)
		{
			HandleSignalForIndex(i, signalType);
		}
	}
}

// Remove an executor from the array
void impl_ns::ExecutionManager::ExecutorThread::RemoveExecutor(ExecutorID execId)
{
	auto itExec = m_signalMap.find(execId);

	if (itExec != m_signalMap.end())
	{
		size_t execIdx = itExec->second;
		auto& rExec = m_executors[execIdx];

		rExec.pExecutor->DeInit();

		size_t lastIdx = m_executors.size() - 1;
		if (execIdx != lastIdx)
		{
			// Swap this with the last one, and update its entry
			auto& rOtherExec = m_executors[lastIdx];

			// Here it's perfectly valid to use [] since the execID is guaranteed to be
			// in the map
			std::swap(rOtherExec, rExec);
			m_signalMap[rOtherExec.execId] = execIdx;
		}

		m_signalMap.erase(execId);
		// An executor must be active in order to signal that it has ended
		// Thus its removal always decreases the active count
		--m_nActive;
		// The executor in question is just the last entry in the array
		m_executors.pop_back();
	}
}

void impl_ns::ExecutionManager::ExecutorThread::PostAddExecutor(ExecutorID execId,
	std::shared_ptr<IExecutor> pExecutor)
{
	ExecutionInstructionMessage execInstruction;
	execInstruction.execId = execId;
	execInstruction.executor = pExecutor;
	execInstruction.instructionTag = ExecutionInstructionTag::AddExecutor;

	std::unique_lock lk{ m_mutexInstructions };
	m_instructionList.emplace_back(execInstruction);
	m_cvInstructions.notify_one();

}
void impl_ns::ExecutionManager::ExecutorThread::PostSignalExecutor(ExecutorID execId,
	ExecutorSignalType signalType)
{
	ExecutionInstructionMessage execInstruction;
	execInstruction.execId = execId;
	execInstruction.instructionTag = ExecutionInstructionTag::SignalExecutor;
	execInstruction.signalType = signalType;

	std::unique_lock lk{ m_mutexInstructions };
	m_instructionList.emplace_back(execInstruction);
	m_cvInstructions.notify_one();
}

void impl_ns::ExecutionManager::ExecutorThread::PostSignalAll(ExecutorSignalType signalType)
{
	ExecutionInstructionMessage execInstruction;
	execInstruction.execId = EXEC_WILDCARD;
	execInstruction.instructionTag = ExecutionInstructionTag::SignalExecutor;
	execInstruction.signalType = signalType;

	std::unique_lock lk{ m_mutexInstructions };
	m_instructionList.emplace_back(execInstruction);
	m_cvInstructions.notify_one();
}

void impl_ns::ExecutionManager::ExecutorThread::DoomMe()
{
	std::unique_lock lk{ m_mutexInstructions };
	m_isDoomed = true;
}

impl_ns::ExecutionManager::ExecutorThread::ExecutorThread(ThreadID id,
	const std::string& threadName,
	ExecutorID execId,
	std::shared_ptr<IExecutor> pFirstExecutor)
	:m_threadId(id),
	m_threadName(threadName),
	m_thread(&ExecutionManager::ExecutorThread::operator(), this, execId, pFirstExecutor)
{
	
}

impl_ns::ExecutionManager::ExecutionManager()
	:m_idleTimeout(SingletonConfig::GetInstance().GetDurationUS("execution_microsecond_wait"))
{

}

void impl_ns::ExecutionManager::LockShop(bool sendTermination)
{
	std::unique_lock lk{ m_mutex };
	
	// Lock up shop by dooming all threads and requesting termination
	for (const auto& thread : m_threadMap)
	{
		thread.second->DoomMe();
		if (sendTermination)
		{
			thread.second->PostSignalAll(ExecutorSignalType::RequestTermination);
		}
	}
	m_lockedShop = true;
	m_cvExecution.notify_all();
}

void impl_ns::ExecutionManager::RunLoop(std::chrono::microseconds wakeUpTime)
{
	std::shared_lock lk{ m_mutex };

	while (!m_lockedShop || !m_threadMap.empty())
	{
		m_cvExecution.wait_for(lk, wakeUpTime);
	}
}

impl_ns::ExecutionManager::~ExecutionManager()
{
	{
		std::shared_lock lk{ m_mutex };

		if (!m_lockedShop)
		{
			std::cerr << "Abnormal termination of ExecutionManager: shop was open";
			std::terminate();
		}

		if (!m_threadMap.empty())
		{
			std::cerr << "Abnormal termination of ExecutionManager: active threads";
			std::terminate();
		}
	}

	for (auto& pThread : m_threadsToJoin)
	{
		pThread->JoinMe();
	}

}

void impl_ns::ExecutionManager::ExecutorThread::JoinMe()
{
	m_thread.join();
}

impl_ns::ExecutorID impl_ns::ExecutionManager::AddExecutor(const char* pThreadName,
	const std::shared_ptr<IExecutor>& pExecutor)
{
	std::unique_lock lk{ m_mutex };

	if (m_nextExec == EXEC_WILDCARD - 1)
	{
		throw ExecutorStateException();
	}

	if (m_lockedShop)
	{
		// Locked shop -- can no longer add threads or executors
		return EXEC_WILDCARD;
	}

	std::string sThreadName;

	// Assign a new ID to the executor
	ExecutorID execId = m_nextExec++;

	ThreadID threadId{ 0 };
	auto itThreadID = m_threadNameMap.find(sThreadName);

	if (itThreadID != m_threadNameMap.end())
	{
		threadId = itThreadID->second;
		// Existing thread.  Send an add message
		auto itThread = m_threadMap.find(threadId);
		itThread->second->PostAddExecutor(execId, pExecutor);
	}
	else
	{
		// Create a new thread
		auto pThread = std::make_shared<ExecutorThread>(m_nextThread,
			sThreadName,
			execId,
			pExecutor);
		threadId = m_nextThread;
		++m_nextThread;
	}

	// Add to the executor map
	m_executorMap.emplace(execId, threadId);

	m_cvExecution.notify_all();

	return execId;
}

void impl_ns::ExecutionManager::DoomThread(const char* pThreadName)
{
	std::unique_lock lk{ m_mutex };
	auto itThreadID = m_threadNameMap.find(pThreadName);

	if (itThreadID != m_threadNameMap.end())
	{
		auto itThread = m_threadMap.find(itThreadID->second);
		
		itThread->second->DoomMe();
		m_threadNameMap.erase(itThreadID);
	}
	m_cvExecution.notify_all();
}

bool impl_ns::ExecutionManager::SignalExecutor(ExecutorID executorId, ExecutorSignalType signalType)
{
	std::shared_lock lk{ m_mutex };

	// Get the thread ID of this executor
	auto itThreadID = m_executorMap.find(executorId);

	if (itThreadID != m_executorMap.end())
	{
		auto itThread = m_threadMap.find(itThreadID->second);
		itThread->second->PostSignalExecutor(executorId, signalType);
		return true;
	}

	return false;
}

void impl_ns::ExecutionManager::RemoveExecutors(const std::vector<ExecutorID>& toRemove)
{
	std::unique_lock lk{ m_mutex };
	
	// Remove executors from the map
	for (ExecutorID execId : toRemove)
	{
		m_executorMap.erase(execId);
	}

	m_cvExecution.notify_all();
}

void impl_ns::ExecutionManager::RemoveThread(ThreadID threadId)
{
	std::unique_lock lk{ m_mutex };

	auto itThread = m_threadMap.find(threadId);

	auto pThread = itThread->second;

	m_threadMap.erase(itThread);
	m_threadNameMap.erase(pThread->GetThreadName());
	// No need to touch the executor map -- a thread is not dead until all executors are removed,
	// and then it is irrevocably dead

	// Move the thread to the "to join" category
	m_threadsToJoin.emplace_back(pThread);

	if (m_threadMap.empty())
	{
		// This works on the assumption that execution should finish when all threads exit
		// This could of course be wrong -- the controlling thread may want to repopulate the 
		// execution manager.  But unless this is done, shutdown is not safe.
		m_lockedShop = true;
	}
	m_cvExecution.notify_all();
}

impl_ns::ExecutionManager& impl_ns::ExecutionManager::GetInstance()
{
	static ExecutionManager me;
	return me;
}
