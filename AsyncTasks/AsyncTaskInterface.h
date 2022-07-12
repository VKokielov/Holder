#pragma once

namespace atask
{

	enum class TaskState
	{
		Continue,
		Complete
	};

	class ITask
	{
	public:
		virtual TaskState Run() = 0;
	};

}