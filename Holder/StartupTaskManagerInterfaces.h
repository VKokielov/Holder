#pragma once

#include <cinttypes>
#include <memory>
#include "IAppObject.h"

namespace holder::base::startup
{
	enum class TaskState : uint8_t
	{
		Unknown = 0,
		Undefined = 1,
		DepsUnknown = 2,
		WaitingForDeps = 3,
		Ready = 4,
		Running = 5,
		Completed = 6,
		Failed = 7
	};

	enum class RequestType
	{
		Define,
		SetDeps,
		Run,
		Complete,
		Fail
	};

	enum class RequestFailType
	{
		InvalidTaskID,
		InvalidTaskState
	};

	using StartupTaskID = uint32_t;

	constexpr StartupTaskID INVALID_TASK = 0xFFFFFF;

	class ITaskResult : public IAppObject
	{

	};

	class ITaskStateAccessor
	{
	public:
		virtual std::shared_ptr<ITaskResult>
			GetTaskResult(const char* taskName) = 0;
		virtual TaskState GetTaskState(const char* taskName) = 0;
	};

	class ITaskStateListener
	{
	public:
		// Called when a task is ready to execute (all dependencies complete).  Set the state to Running and do your thing!
		virtual void OnTaskReady(StartupTaskID taskId, ITaskStateAccessor& taskStates) = 0;
		// Called when at least one task in the dependency list of another has failed.
		// Mark the task failed or update the dependencies
		virtual void OnDependencyFailed(StartupTaskID taskId,
			const char* dependencyName,
			ITaskStateAccessor& taskStates) = 0;
		// Called when a request for some task fails
		virtual void OnRequestFailed(StartupTaskID taskId,
			RequestType requestType,
			RequestFailType failType) = 0;
		virtual ~ITaskStateListener() = 0;
	};
}