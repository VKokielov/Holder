// Holder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "AppLibrary.h"
#include "IStarter.h"
#include "AppArgumentHelper.h"

#include <memory>
#include <vector>
#include <string>

int main(int argc, const char** argv)
{
	using namespace holder;
	// Get a factory for the starter based on the first argument
	if (argc < 2)
	{
		std::cerr << "ERROR (Holder): missing starter factory name!\n";
		return -1;
	}

	const char* pStarterFactoryName = argv[1];

	std::shared_ptr<base::IAppObjectFactory> pStarterFactory;
	if (!base::AppLibrary::GetInstance().FindFactory(pStarterFactoryName,
		pStarterFactory))
	{
		std::cerr << "ERROR (Holder): starter factory " << pStarterFactoryName << " not found; exiting.\n";
		return -1;
	}

	class StarterArgs : public 
		base::AppArgumentHelper<StarterArgs, service::IStarterArgument>
	{
	public:
		StarterArgs(int argc, const char** argv)
		{
			for (int i = 0; i < argc; i++)
			{
				m_args.emplace_back(argv[i]);
			}
		}
		size_t GetArgCount() const override { return m_args.size(); }

		const char* GetArgument(size_t index) const override 
		{ 
			if (index < m_args.size()) 
			{
				return m_args[index].c_str();
			} 
			return nullptr;
		}

	private:
		std::vector<std::string>
			m_args;
	};

	StarterArgs starterArgs(argc, argv);

	// Create the starter
	std::shared_ptr<base::IAppObject> pStarterRaw(pStarterFactory->Create(starterArgs));

	std::shared_ptr<service::IServiceStarter> pStarter (std::dynamic_pointer_cast<service::IServiceStarter>(pStarterRaw));
	
	if (!pStarter)
	{
		std::cerr << "ERROR (Holder): Starter factory " << pStarterFactoryName << " did not produce a valid starter.\n";
		return -1;
	}

	if (!pStarter->Start())
	{
		std::cerr << "ERROR (Holder): Starter::Start failed.\n";
		return -1;
	}

	pStarter->Run();

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
