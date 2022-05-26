#pragma once
#include "IAppObject.h"

#include <cinttypes>

namespace holder::base
{

	// User-provided
	using TimerUserID = uint32_t;
	// For anonymous timers this is how the timer is identified
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
		virtual bool Init() = 0;
		virtual void DeInit() = 0;
		virtual ExecutionState Run(ExecutionArgs& args) = 0;
		virtual void TerminationRequested() = 0;
	};

	class ITimerCallback : public IAppObject
	{
	public:
		virtual void OnTimer(TimerUserID timerUserId, TimerID timerID) = 0;
	};

}