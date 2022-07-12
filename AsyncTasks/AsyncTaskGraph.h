#pragma once

#include "AsyncTaskInterface.h"

#include <memory>
#include <cinttypes>
#include <unordered_set>
#include <mutex>
#include <optional>
#include <algorithm>

namespace atask
{
	class AsyncGraphBuilder;

	class AsyncGraphException { };

	using TaskID = uint64_t;

	class AsyncGraphOrder
	{
	private:
		struct TaskState
		{
			std::shared_ptr<ITask> pTask;
			std::vector<TaskID> dependentTasks;
			// Number of *my* outstanding dependencies
			TaskID originalDependencyCount{ 0 };
			TaskID dependencyCount{ 0 };
		};

		AsyncGraphOrder(const AsyncGraphBuilder& builder);

	public:
		bool HasUnexecutedTasks() const
		{
			return m_completedTasks < m_taskStates.size();
		}

		// Task manipulation
		void Reset();

		// NOTE:  Calling these functions entails a promise to run the tasks
		template<typename F>
		void GetInitialTasksToExecute(F&& taskCallback)
		{
			// Initial tasks are those which have no dependencies

			for (TaskID taskId : m_initialTasks)
			{
				++m_executingTasks;
				taskCallback(taskId, m_taskStates[taskId].pTask);
			}
		}

		// Note:  Task 0 is always the "initial task" on which all tasks depend
		template<typename F, typename I>
		void GetTasksToExecute(I itBeginComplete, I itEndComplete,
			F&& taskCallback)
		{ 
			// Initial tasks are those which have no dependencies

			for (I itComplete = itBeginComplete;
				itComplete != itEndComplete;
				++itComplete)
			{
				TaskID taskId = *itComplete;
				--m_executingTasks;

				for (TaskID taskDepId : m_taskStates[taskId].dependentTasks)
				{
					--m_taskStates[taskDepId].dependencyCount;
					if (m_taskStates[taskDepId].dependencyCount == 0)
					{
						++m_executingTasks;
						taskCallback(taskDepId, m_taskStates[taskDepId].pTask);
					}
				}
			}
		}
	private:
		std::vector<TaskID> m_initialTasks;
		std::vector<TaskState> m_taskStates;
		unsigned int m_completedTasks{ 0 };
		unsigned int m_executingTasks{ 0 };

		friend class AsyncGraphBuilder;
	};

	class AsyncGraphBuilder
	{
	private:
		struct Vertex_
		{
			std::shared_ptr<ITask> pTask;
			std::unordered_set<TaskID> vFrom;
			std::unordered_set<TaskID> vTo;
		};

	public:
		TaskID AddTask(std::shared_ptr<ITask> pTask);

		void ClearDependencies(TaskID taskID);
		void AddDependency(TaskID fromID, TaskID toID);

		AsyncGraphOrder ConstructGraph();

	private:
		std::vector<Vertex_> m_vertices;

		friend class AsyncGraphOrder;
	};

}