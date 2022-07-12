// AsyncTasks.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>

#include "AsyncExecutor.h"

class TestTask : public atask::ITask
{
public:
	TestTask(unsigned int delay,
		unsigned int iterations,
		const char* label)
		:m_delay(delay),
		m_label(label),
		m_iterations(iterations),
		m_curIterations(m_iterations)
	{ }

	void Reset()
	{
		m_curIterations = m_iterations;
	}

	atask::TaskState Run()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(m_delay));
		--m_curIterations;
		if (m_curIterations == 0)
		{
			std::cout << m_label << ": done!\n";
			return atask::TaskState::Complete;
		}
		else
		{
			std::cout << m_label << ": iterations left " << m_curIterations << "\n";
			return atask::TaskState::Continue;
		}
	}

private:
	unsigned int m_delay;
	std::string m_label;
	unsigned int m_iterations;
	unsigned int m_curIterations;
};

int main()
{
    std::cout << "Hello World!\n";

	static atask::AsyncTaskExecutor asyncExec(5);
	

	auto pTaskA0 = std::make_shared<TestTask>(500, 1, "taskA0");
	auto pTaskB0 = std::make_shared<TestTask>(250, 2, "taskB0");
	auto pTaskC0 = std::make_shared<TestTask>(750, 1, "taskC0");
	
	auto pTaskA1 = std::make_shared<TestTask>(500, 1, "taskA1");
	auto pTaskA2 = std::make_shared<TestTask>(500, 1, "taskA2");

	std::vector<std::shared_ptr<TestTask> > testTasks{ pTaskA0, pTaskB0, pTaskC0, pTaskA1, pTaskA2 };

	// Construct a graph
	atask::AsyncGraphBuilder graphBuilder;
	auto tidA0 = graphBuilder.AddTask(pTaskA0);
	auto tidB0 = graphBuilder.AddTask(pTaskB0);
	auto tidC0 = graphBuilder.AddTask(pTaskC0);
	auto tidA1 = graphBuilder.AddTask(pTaskA1);
	auto tidA2 = graphBuilder.AddTask(pTaskA2);

	// A0 B0 C0
	// \  /  /
	//  A1  /
	//  | /
	//  A2

	graphBuilder.AddDependency(tidA1, tidA0);
	graphBuilder.AddDependency(tidA1, tidB0);
	graphBuilder.AddDependency(tidA2, tidA1);
	graphBuilder.AddDependency(tidA2, tidC0);

	while (true)
	{
		atask::AsyncGraphOrder graphOrder = graphBuilder.ConstructGraph();

		// Execute!
		asyncExec.ExecuteGraph(graphOrder);

		std::cout << "All tasks complete -- starting up again...\n";

		// Reset
		for (auto pTestTask : testTasks)
		{
			pTestTask->Reset();
		}
	}

}

