#pragma once
#include "IAppObject.h"

#include <cinttypes>

namespace holder::base
{

	// User-provided
	using TimerID = uint32_t;

	enum class ExecutionState
	{
		Continue,
		Suspend,  // until signalled
		End
	};

	struct ExecutionArgs 
	{ };

	class IExecutor : public IAppObject
	{
	public:
		virtual void Init() = 0;
		virtual void DeInit() = 0;
		virtual ExecutionState Run(ExecutionArgs& args) = 0;
		virtual void TerminationRequested() = 0;
	};

	class ITimerCallback : public IAppObject
	{
	public:
		virtual void OnTimer(TimerID timerId) = 0;
	};

}