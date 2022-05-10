#pragma once
#include "IAppObject.h"

namespace holder::base
{

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

}