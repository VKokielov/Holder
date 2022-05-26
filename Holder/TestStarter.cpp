#include "TestStarter.h"
#include "RegistrationHelperAliases.h"
#include "BaseServiceConfiguration.h"

#include <iostream>

namespace impl_ns = holder::test;

namespace
{
	holder::StarterRegistrationHelper<impl_ns::TestStarter>
		g_starterRegistration("/starters/TestStarter");
}

impl_ns::TestStarter::TestStarter(const holder::service::IStarterArgument& starterArgument)
{
	// Print the arguments
	std::cout << "Arguments:\n";
	for (size_t i = 0; i < starterArgument.GetArgCount(); ++i)
	{
		std::cout << "\t" << starterArgument.GetArgument(i) << '\n';
	}

	AddService(std::make_shared<service::BaseServiceConfiguration>("/root/services/ConsoleService",
					"/services/ConsoleTextService", 
					"threadA", true));

	AddService(std::make_shared<service::BaseServiceConfiguration>("/root/services/TestServiceAlpha",
		"/services/TestServiceAlpha",
		"threadB", true));
}

	