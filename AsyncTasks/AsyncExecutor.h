#pragma once

#include "AsyncTaskInterface.h"
#include "AsyncTaskGraph.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <shared_mutex>

namespace atask
{

	enum class TaskResult
	{
		ExecutionComplete,
		// If this is returned, there was a dependency in the graph
		UnableToProgress 
	};

	enum class TaskWaitResult
	{
		OK,
		UnableToProgress
	};

	class AsyncTaskExecutor
	{
	private:
		// The task context is local to the execution and
		// is guaranteed to exist 
		
		class TaskContext;

		struct PackagedTask
		{
			TaskID taskId;
			std::shared_ptr<ITask> pTask;
			size_t contextId;

			PackagedTask(TaskID taskId_,
				std::shared_ptr<ITask> pTask_,
				size_t contextId_)
				:taskId(taskId_),
				pTask(pTask_),
				contextId(contextId_)
			{ }
		};

		struct TaskCompletionMessage
		{
			size_t contextId;
			TaskID taskId;

			TaskCompletionMessage(size_t contextId_, 
				TaskID taskId_)
				:contextId(contextId_),
				taskId(taskId_)
			{ }
		};

		class TaskExecutorThread
		{
		public:
			TaskExecutorThread(AsyncTaskExecutor& m_owner);

			void Join();
			void PostTask(PackagedTask&& task);

			unsigned long AtomicGetLoad() const
			{
				return m_taskLoad.load();
			}

		private:
			void Run();

			AsyncTaskExecutor& m_owner;
			std::unique_ptr<std::thread> m_thread;
			std::mutex m_mutex;
			std::condition_variable m_cv;
			std::deque<PackagedTask> m_todoList;
			std::atomic<unsigned long> m_taskLoad{ 0 };
			std::atomic<bool> m_stopFlag{ false };
		};

		class TaskContext
		{
		private:
			class TaskListAcceptor
			{
			public:
				TaskListAcceptor(size_t contextId)
					:m_contextId(contextId)
				{ }

				void operator()(TaskID taskId, 
					std::shared_ptr<ITask> pTask)
				{
					taskList.emplace_back(taskId, pTask, m_contextId);
				}

				const size_t m_contextId;
				std::vector<PackagedTask> taskList;
			};

		public:
			TaskContext(AsyncTaskExecutor& executor,
				size_t contextId)
				:m_owner(executor),
				m_contextId(contextId)
			{ }

			size_t GetContextID() const { return m_contextId; }
			TaskResult RunAllTasks(AsyncGraphOrder& graph);

			void PostCompletionMessage(TaskCompletionMessage&& msg);

		private:

			std::mutex m_mutex;
			std::condition_variable m_cv;

			// Incoming queue of completion messages
			std::deque<TaskCompletionMessage> m_completionQueue;

			// These are NOT shared and are not protected by the mutex!
			AsyncTaskExecutor& m_owner;
			size_t m_contextId;
		};

	public:
		AsyncTaskExecutor(size_t threadCount);
		~AsyncTaskExecutor();

		TaskResult ExecuteGraph(AsyncGraphOrder& taskOrder);
	private:
		TaskContext& CreateContext();
		void DeleteContext(size_t contextId);

		void PostTaskCompletions(const std::vector<TaskCompletionMessage>& taskCompletions);
		void PostTaskToThread(PackagedTask&& task);

		std::shared_mutex m_mutex;
		std::condition_variable m_cv;

		std::deque<TaskCompletionMessage> m_taskCompletions;
		std::vector<std::unique_ptr<TaskExecutorThread> > m_threads;
		
		std::unordered_map<size_t, TaskContext> m_taskContexts;
		size_t m_freeContextId{ 0 };

	};

}