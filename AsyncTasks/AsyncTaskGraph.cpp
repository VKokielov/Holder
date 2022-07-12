#include "AsyncTaskGraph.h"

#include <unordered_map>


atask::AsyncGraphOrder::AsyncGraphOrder(const AsyncGraphBuilder& builder)
{
	m_taskStates.resize(builder.m_vertices.size());

	for (TaskID taskId = 0;
		taskId < builder.m_vertices.size();
		++taskId)
	{
		// Copy dependencies and set dependency counts
		auto& taskStateOrder = m_taskStates[taskId];

		const auto& builderTask = builder.m_vertices[taskId];

		taskStateOrder.dependentTasks.resize(builderTask.vTo.size());

		for (TaskID depTask : builderTask.vTo)
		{
			taskStateOrder.dependentTasks.emplace_back(depTask);
		}
		
		taskStateOrder.dependencyCount 
			= taskStateOrder.originalDependencyCount
			= builderTask.vFrom.size();

		if (taskStateOrder.dependencyCount == 0)
		{
			m_initialTasks.emplace_back(taskId);
		}
		// Save the callback
		taskStateOrder.pTask = builderTask.pTask;
	}
}

void atask::AsyncGraphOrder::Reset()
{
	// Bring the graph back to usable state

	m_completedTasks = 0;
	m_executingTasks = 0;

	for (auto& taskStateOrder : m_taskStates)
	{
		taskStateOrder.dependencyCount = taskStateOrder.originalDependencyCount;
	}
}


atask::TaskID atask::AsyncGraphBuilder::AddTask(std::shared_ptr<ITask> pTask)
{
	auto nextID = (TaskID)m_vertices.size();

	m_vertices.emplace_back();
	m_vertices.back().pTask = pTask;

	return nextID;
}

void atask::AsyncGraphBuilder::ClearDependencies(TaskID taskID)
{

	if (taskID >= m_vertices.size())
	{
		throw AsyncGraphException();
	}

	// The data structures are unordered_sets so duplicates are no-ops
	auto& builderTask = m_vertices[taskID];

	for (TaskID depId : builderTask.vFrom)
	{
		m_vertices[depId].vTo.erase(taskID);
	}

	builderTask.vFrom.clear();
}

void atask::AsyncGraphBuilder::AddDependency(TaskID fromId, TaskID toId)
{
	if (fromId >= m_vertices.size() || toId >= m_vertices.size())
	{
		throw AsyncGraphException();
	}

	auto& fromTask = m_vertices[fromId];
	auto& toTask = m_vertices[toId];

	fromTask.vFrom.emplace(toId);
	toTask.vTo.emplace(fromId);
}

atask::AsyncGraphOrder atask::AsyncGraphBuilder::ConstructGraph()
{
	return AsyncGraphOrder(*this);
}