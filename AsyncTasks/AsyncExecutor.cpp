#include "AsyncExecutor.h"

atask::AsyncTaskExecutor::TaskExecutorThread::TaskExecutorThread(AsyncTaskExecutor& owner)
	:m_owner(owner)
{
	m_thread.reset (new std::thread(&TaskExecutorThread::Run, this));
}
void atask::AsyncTaskExecutor::TaskExecutorThread::Run()
{
	std::deque<PackagedTask> todoList;
	std::deque<PackagedTask> unfinishedBusiness;

	while (!m_stopFlag.load())
	{
		std::vector<TaskCompletionMessage> completionMessages;
		completionMessages.clear();

		todoList.clear();
		{
			std::unique_lock lk{ m_mutex };
			while (m_todoList.empty())
			{
				m_cv.wait(lk);
			}

			std::swap(todoList, m_todoList);
		}

		while (!todoList.empty())
		{
			PackagedTask& currentTask = todoList.front();

			auto taskRunResult = currentTask.pTask->Run();

			if (taskRunResult == TaskState::Complete)
			{
				completionMessages.emplace_back(currentTask.contextId, currentTask.taskId);
				--m_taskLoad;
				todoList.pop_front();
			}
			else
			{
				unfinishedBusiness.push_back(std::move(currentTask));
				todoList.pop_front();
			}
		}

		// Process only as much as is in the queue right now
		size_t numToProcess = unfinishedBusiness.size();
		size_t numProcessed = 0;
		while (numProcessed < numToProcess)
		{
			PackagedTask& currentTask = unfinishedBusiness.front();

			auto taskRunResult = currentTask.pTask->Run();

			if (taskRunResult == TaskState::Complete)
			{
				completionMessages.emplace_back(currentTask.contextId, currentTask.taskId);
				--m_taskLoad;
			}
			else
			{
				unfinishedBusiness.push_back(std::move(currentTask));
			}
			unfinishedBusiness.pop_front();
			++numProcessed;
		}

		m_owner.PostTaskCompletions(completionMessages);
	}
}

void atask::AsyncTaskExecutor::TaskExecutorThread::PostTask(PackagedTask&& task)
{
	std::unique_lock lk{ m_mutex };
	m_todoList.push_back(std::move(task));
	m_cv.notify_one();
	++m_taskLoad;
}

void atask::AsyncTaskExecutor::TaskExecutorThread::Join()
{
	m_stopFlag.store(true);
	m_thread->join();
}

atask::AsyncTaskExecutor::AsyncTaskExecutor(size_t threadCount)
{
	++threadCount;

	while (threadCount > 0)
	{
		m_threads.emplace_back(new TaskExecutorThread(*this));
		--threadCount;
	}
	
}
atask::AsyncTaskExecutor::~AsyncTaskExecutor()
{
	std::shared_lock lk(m_mutex);
	for (auto& thread : m_threads)
	{
		thread->Join();
	}
}

atask::TaskResult atask::AsyncTaskExecutor::ExecuteGraph(atask::AsyncGraphOrder& graph)
{
	// A context represents a call to this function
	// Contexts are needed for this function to be reentrant from multiple threads
	// Contexts should NOT be accessed from multiple threads except to post
	// task completions

	TaskContext& context = CreateContext();
	auto retVal = context.RunAllTasks(graph);
	
	DeleteContext(context.GetContextID());
	return retVal;
}

atask::TaskResult atask::AsyncTaskExecutor::TaskContext::RunAllTasks(AsyncGraphOrder& graph)
{
	bool isInitial{ true };

	size_t pendingTasks{ 0 };
	TaskListAcceptor tasksToExecute(m_contextId);

	// Begin by getting the next set of tasks to execute
	std::deque<TaskCompletionMessage> completionQueue;

	graph.GetInitialTasksToExecute(tasksToExecute);
	pendingTasks += tasksToExecute.taskList.size();

	while (pendingTasks != 0)
	{
		// Post
		for (PackagedTask& pendingTask : tasksToExecute.taskList)
		{
			m_owner.PostTaskToThread(std::move(pendingTask));
		}
		tasksToExecute.taskList.clear();

		// Wait for completion
		{
			std::unique_lock lk(m_mutex);

			while (m_completionQueue.empty())
			{
				m_cv.wait(lk);
			}

			completionQueue.clear();
			std::swap(completionQueue, m_completionQueue);
		}

		pendingTasks -= completionQueue.size();

		// Add pending dependencies
		while (!completionQueue.empty())
		{
			graph.GetTasksToExecute(completionQueue.front().taskId, tasksToExecute);
			completionQueue.pop_front();
		}
		pendingTasks += tasksToExecute.taskList.size();
	}

	return !graph.HasUnexecutedTasks() ? TaskResult::ExecutionComplete : TaskResult::UnableToProgress;
}

void atask::AsyncTaskExecutor::TaskContext::PostCompletionMessage(TaskCompletionMessage&& msg)
{
	std::unique_lock lk{ m_mutex };
	m_completionQueue.push_back(std::move(msg));
	// Notify the waiting thread
	m_cv.notify_one();
}

void atask::AsyncTaskExecutor::PostTaskCompletions(const std::vector<TaskCompletionMessage>& taskCompletions)
{
	std::shared_lock lk(m_mutex);

	for (const TaskCompletionMessage& msg : taskCompletions)
	{
		auto itContext = m_taskContexts.find(msg.contextId);
		if (itContext != m_taskContexts.end())
		{
			// Post to the context
			itContext->second.PostCompletionMessage(TaskCompletionMessage(msg));
		}
	}

}
void atask::AsyncTaskExecutor::PostTaskToThread(PackagedTask&& task)
{
	// Go through the threads and pick the one with the smallest load
	TaskExecutorThread* pThread{ nullptr };
	{
		unsigned long smallestLoad{ 100000000 };
		std::shared_lock lk(m_mutex);

		pThread = m_threads.front().get();

		for (size_t i = 0; i < m_threads.size(); ++i)
		{
			auto threadLoad = m_threads[i]->AtomicGetLoad();
			if (threadLoad < smallestLoad)
			{
				smallestLoad = threadLoad;
				pThread = m_threads[i].get();
			}
		}
	}

	// Post
	pThread->PostTask(std::move(task));
}

atask::AsyncTaskExecutor::TaskContext& atask::AsyncTaskExecutor::CreateContext()
{
	std::unique_lock lk{m_mutex};
	size_t nextId = m_freeContextId++;

	auto emplResult = m_taskContexts.emplace(std::piecewise_construct,
		std::forward_as_tuple(nextId),
		std::forward_as_tuple(*this, nextId));

	return emplResult.first->second;
}
void atask::AsyncTaskExecutor::DeleteContext(size_t contextId)
{
	std::unique_lock lk{ m_mutex };
	m_taskContexts.erase(contextId);
}
