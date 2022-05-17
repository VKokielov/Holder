#include "StartupTaskManager.h"

namespace impl_ns = holder::base::startup;

bool impl_ns::StartupTaskManager::StartupTaskExecutor::Init()
{ 
	return true;
}

void impl_ns::StartupTaskManager::StartupTaskExecutor::DeInit()
{ }

namespace
{
	const char* startupThread_ = "/infra/StartupBackgroundThread";
}

holder::base::ExecutionState impl_ns::StartupTaskManager::StartupTaskExecutor::Run(holder::base::ExecutionArgs& args)
{
	{
		StartupTaskManager& mgrInstance = StartupTaskManager::GetInstance();
		std::unique_lock lk{ mgrInstance.m_mutex };

		if (m_stopRequested)
		{
			mgrInstance.m_running = false;
			return ExecutionState::End;
		}

		std::swap(m_localQueue, mgrInstance.m_executorQueue);
	}


	while (!m_localQueue.empty())
	{
		auto pAction = m_localQueue.back();
		pAction->Act(this);
		m_localQueue.pop_back();
	}

	// Either suspend or end, depending on whether the executor has tasks not in Completed or Failed state, 
	// and should continue empty 
	if (m_stopWhenComplete && !m_taskStates.empty() && m_waitingTasks == 0)
	{
		return ExecutionState::End;
	}


	return ExecutionState::Suspend;
}

void impl_ns::StartupTaskManager::StartupTaskExecutor::TerminationRequested()
{
	m_stopRequested = true;
}

std::shared_ptr<impl_ns::ITaskResult> impl_ns::StartupTaskManager::StartupTaskExecutor::GetTaskResult(const char* taskName)
{
	std::shared_ptr<impl_ns::ITaskResult> retVal{};

	auto itTaskNameDesc = m_taskNameMap.find(std::string(taskName));
	if (itTaskNameDesc == m_taskNameMap.end() || !itTaskNameDesc->second.taskId.has_value())
	{
		return retVal;
	}

	auto itTask = m_taskStates.find(itTaskNameDesc->second.taskId.value());

	if (itTask->second.state != TaskState::Completed
		&& itTask->second.state != TaskState::Failed)
	{
		return retVal;
	}

	retVal = itTask->second.pResult;

	return retVal;
}

impl_ns::TaskState impl_ns::StartupTaskManager::StartupTaskExecutor::GetTaskState(const char* taskName)
{
	auto itTaskNameDesc = m_taskNameMap.find(std::string(taskName));
	if (itTaskNameDesc == m_taskNameMap.end())
	{
		return TaskState::Unknown;
	}

	if (!itTaskNameDesc->second.taskId.has_value())
	{
		return TaskState::Undefined;
	}

	auto itTask = m_taskStates.find(itTaskNameDesc->second.taskId.value());
	return itTask->second.state;
}

void impl_ns::StartupTaskManager::StartupTaskExecutor::EvaluateDependencies(TaskStateDesc_& stateDesc)
{
	// Inform the task of any failures in its dependencies - once they arrive
	// If all deps are complete the task is ready for execution

	unsigned int nComplete{ 0 };
	for (StartupTaskID dependency : stateDesc.dependsOn)
	{
		auto itTask = m_taskStates.find(dependency);
		const TaskStateDesc_& depTaskDesc = itTask->second;

		if (depTaskDesc.state == TaskState::Completed)
		{
			++nComplete;
		}
		else if (depTaskDesc.state == TaskState::Failed
			&& (stateDesc.failuresReported.count(dependency) == 0))
		{
			stateDesc.pListener->OnDependencyFailed(stateDesc.id,
				depTaskDesc.name.c_str(),
				*this);
			stateDesc.failuresReported.emplace(dependency);
		}
	}

	// If there are undefined dependencies the ready-check should never come up true
	if (nComplete == stateDesc.dependsOn.size()
		&& stateDesc.dependsOnUndefined.empty())
	{
		stateDesc.state = TaskState::Ready;
		stateDesc.pListener->OnTaskReady(stateDesc.id, *this);
	}
}


void impl_ns::StartupTaskManager::StartupTaskExecutor::HandleAction(const DefineTaskAction& action)
{
	// NOTE:  By selection, action.GetID() was not previously in the map
	// Also, pListener is non-nullptr (though of course it may be garbage)

	// There are two possibilities.  Either the task is not in the name map, or it is in the name map
	// but undefined
	auto itTaskName = m_taskNameMap.find(action.GetName());

	TaskStateDesc_ taskStateDesc;

	if (itTaskName != m_taskNameMap.end())
	{
		const std::unordered_set<StartupTaskID>& dependingTasks = itTaskName->second.dependedOnBy.value();

		for (StartupTaskID dependentId : dependingTasks)
		{
			auto itDependent = m_taskStates.find(dependentId);
			itDependent->second.dependsOnUndefined.erase(action.GetName());
			itDependent->second.dependsOn.emplace(action.GetID());
		}

		taskStateDesc.dependedOnBy = itTaskName->second.dependedOnBy.value();
		itTaskName->second.dependedOnBy.reset();
		itTaskName->second.taskId.emplace(action.GetID());
	}
	else
	{
		// New task -- add to name map
		auto emplResult = m_taskNameMap.emplace(action.GetName(), TaskNameDesc_());
		emplResult.first->second.taskId.emplace(action.GetID());
	}

	taskStateDesc.id = action.GetID();
	taskStateDesc.name = action.GetName();
	taskStateDesc.state = TaskState::DepsUnknown;
	taskStateDesc.pListener = action.GetListener();
	++m_waitingTasks;

	m_taskStates.emplace(action.GetID(), taskStateDesc);
}

void impl_ns::StartupTaskManager::StartupTaskExecutor::HandleAction(const SetDependenciesAction& action)
{
	// If the dependency list is empty, the task has no dependencies and can immediately be marked
	// "ready"

	auto itTask = m_taskStates.find(action.GetID());

	if (itTask == m_taskStates.end())
	{
		StartupTaskManager::GetInstance().OnInvalidTaskID(action.GetID(), RequestType::SetDeps);
		return;
	}

	TaskStateDesc_& taskState = itTask->second;
	
	if (taskState.state != TaskState::DepsUnknown &&
		taskState.state != TaskState::WaitingForDeps)
	{
		taskState.pListener->OnRequestFailed(taskState.id, RequestType::SetDeps, RequestFailType::InvalidTaskState);
		return;
	}

	// Convert the nonempty dependency list into IDs, creating whatever deps don't exist as "undefined"
	// in the name map
	std::unordered_set<StartupTaskID> dependencies;
	std::unordered_set<std::string> undefinedDependencies;
	unsigned int undefinedDeps{ 0 };

	// This loop collects dependencies.  It does NOT update the objects describing
	// these dependencies; that is the job of the loop following this one.
	for (const std::string& depName : action.GetDeps())
	{
		auto itDep = m_taskNameMap.find(depName);

		if (itDep == m_taskNameMap.end())
		{
			TaskNameDesc_ nameDesc;
			// Create the dependedOnBy set
			nameDesc.dependedOnBy.emplace();
			m_taskNameMap.emplace(depName, nameDesc);
		}
		else
		{
			// Is the task a real one?
			if (itDep->second.taskId.has_value())
			{
				dependencies.emplace(itDep->second.taskId.value());
			}
			else
			{
				// Another undefined dependency.
				undefinedDependencies.emplace(depName);
			}
		}
	}

	// Next, update the existing dependencies and the new dependencies, the long way
	for (StartupTaskID curDep : taskState.dependsOn)
	{
		auto itCurDep = m_taskStates.find(curDep);
		itCurDep->second.dependedOnBy.erase(taskState.id);
	}

	for (const std::string& curUndefDep : taskState.dependsOnUndefined)
	{
		auto itCurDepUndefined = m_taskNameMap.find(curUndefDep);
		itCurDepUndefined->second.dependedOnBy.value().erase(taskState.id);
	}

	for (StartupTaskID newDep : dependencies)
	{
		auto itNewDep = m_taskStates.find(newDep);
		itNewDep->second.dependedOnBy.emplace(taskState.id);
	}

	for (const std::string& newUndefDep : undefinedDependencies)
	{
		auto itNewDepUndefined = m_taskNameMap.find(newUndefDep);
		itNewDepUndefined->second.dependedOnBy.value().emplace(taskState.id);
	}

	// Assign the new values
	taskState.dependsOn = dependencies;
	taskState.dependsOnUndefined = undefinedDependencies;

	// Finally, evaluate the dependencies to decide whether the end state is "Waiting" or "Ready"
	// "Ready" is only possible when all dependencies are in "Completed" state.
	taskState.failuresReported.clear();

	taskState.state = TaskState::WaitingForDeps;
	EvaluateDependencies(taskState);
}

void impl_ns::StartupTaskManager::StartupTaskExecutor::HandleAction(const CompleteTaskAction& action)
{
	// Mark a task as completed 
	auto itTask = m_taskStates.find(action.GetID());

	if (itTask == m_taskStates.end())
	{
		StartupTaskManager::GetInstance().OnInvalidTaskID(action.GetID(), RequestType::Complete);
		return;
	}

	TaskStateDesc_& taskState = itTask->second;

	if (action.GetSuccess())
	{
		if (taskState.state != TaskState::Running)
		{
			taskState.pListener->OnRequestFailed(taskState.id, RequestType::Complete, RequestFailType::InvalidTaskState);
			return;
		}
	}
	else
	{
		if (taskState.state != TaskState::Running
			&& taskState.state != TaskState::Ready
			&& taskState.state != TaskState::WaitingForDeps)
		{
			taskState.pListener->OnRequestFailed(taskState.id, RequestType::Complete, RequestFailType::InvalidTaskState);
			return;
		}
	}

	// Update the task result and state
	taskState.pResult = action.GetResult();
	if (action.GetSuccess())
	{
		taskState.state = TaskState::Completed;
	}
	else
	{
		taskState.state = TaskState::Failed;
	}
	--m_waitingTasks;

	// Now evaluate the dependencies of each task depending on this one
	// TODO:  Optimize the solution to use counters and avoid reevaluating the entire state
	for (StartupTaskID dependent : taskState.dependedOnBy)
	{
		auto itDep = m_taskStates.find(dependent);
		EvaluateDependencies(itDep->second);
	}

}

void impl_ns::StartupTaskManager::StartupTaskExecutor::HandleAction(const RunTaskAction& action)
{
	// Mark a task as running if it's ready to run
	auto itTask = m_taskStates.find(action.GetID());

	if (itTask == m_taskStates.end())
	{
		StartupTaskManager::GetInstance().OnInvalidTaskID(action.GetID(), RequestType::Run);
		return;
	}

	TaskStateDesc_& taskState = itTask->second;	
	if (taskState.state != TaskState::Ready)
	{
		taskState.pListener->OnRequestFailed(taskState.id, RequestType::Run, RequestFailType::InvalidTaskState);
		return;
	}

	// Setting the task to "running" doesn't change anything for the tasks that depend on it,
	// so this code is very simple
	taskState.state = TaskState::Running;
}

impl_ns::StartupTaskID impl_ns::StartupTaskManager::DefineStartupTask(const char* taskName,
	std::shared_ptr<impl_ns::ITaskStateListener> pListener)
{
	if (!pListener)
	{
		return impl_ns::INVALID_TASK;
	}

	// No point optimizing task definitions for local execution
	std::unique_lock lk{ m_mutex };

	StartupTaskID newTaskId = m_nextTaskId++;

	auto pCreateTask = std::make_shared<DefineTaskAction>(newTaskId, std::string(taskName), pListener);
	m_executorQueue.emplace_front(std::move(pCreateTask));

	if (m_running)
	{
		ExecutionManager::GetInstance().SignalExecutor(m_execID,
			ExecutorSignalType::WakeUp);
	}

	return newTaskId;
}
void impl_ns::StartupTaskManager::SetStartupDependencies(StartupTaskID taskId, const std::vector<std::string>& deps)
{
	std::unordered_set<std::string> depSet(deps.begin(), deps.end());

	if (ExecutionManager::GetCurrentThreadName()
		== startupThread_)
	{
		SetDependenciesAction taskAction(taskId, depSet);
		m_pMyExecutor->HandleAction(taskAction);
		return;
	}

	std::unique_lock lk{ m_mutex };
	auto pSetDeps = std::make_shared<SetDependenciesAction>(taskId, depSet);
	m_executorQueue.emplace_front(std::move(pSetDeps));

	if (m_running)
	{
		ExecutionManager::GetInstance().SignalExecutor(m_execID,
			ExecutorSignalType::WakeUp);
	}
}

void impl_ns::StartupTaskManager::RunTask(StartupTaskID taskId)
{
	if (ExecutionManager::GetCurrentThreadName()
		== startupThread_)
	{
		RunTaskAction taskAction(taskId);
		m_pMyExecutor->HandleAction(taskAction);
		return;
	}

	std::unique_lock lk{ m_mutex };
	auto pRunTask = std::make_shared<RunTaskAction>(taskId);
	m_executorQueue.emplace_front(std::move(pRunTask));

	if (m_running)
	{
		ExecutionManager::GetInstance().SignalExecutor(m_execID,
			ExecutorSignalType::WakeUp);
	}
}
void impl_ns::StartupTaskManager::CompleteTask(StartupTaskID taskId, bool success, std::shared_ptr<ITaskResult> pResult)
{
	if (ExecutionManager::GetCurrentThreadName()
		== startupThread_)
	{
		CompleteTaskAction taskAction(taskId, success, pResult);
		m_pMyExecutor->HandleAction(taskAction);
		return;
	}

	std::unique_lock lk{ m_mutex };
	auto pCompleteTask = std::make_shared<CompleteTaskAction>(taskId, success, pResult);
	m_executorQueue.emplace_front(std::move(pCompleteTask));

	if (m_running)
	{
		ExecutionManager::GetInstance().SignalExecutor(m_execID,
			ExecutorSignalType::WakeUp);
	}
}

void impl_ns::StartupTaskManager::StartExecutor(bool stopWhenComplete)
{
	std::unique_lock lk{ m_mutex };
	if (!m_running)
	{
		m_pMyExecutor
			= std::make_shared<StartupTaskExecutor>(stopWhenComplete);
		// Start the startup thread
		// "singleton" means this executor has this thread exclusively
		m_execID = ExecutionManager::GetInstance().AddExecutor(startupThread_, m_pMyExecutor, true);
		m_running = true;
	}
}

impl_ns::StartupTaskManager& impl_ns::StartupTaskManager::GetInstance()
{
	static StartupTaskManager me;
	return me;
}

void impl_ns::StartupTaskManager::OnInvalidTaskID(StartupTaskID taskId, RequestType requestType)
{

}