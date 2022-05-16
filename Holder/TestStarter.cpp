#include "TestStarter.h"
#include "RegistrationHelperAliases.h"
#include "BaseServiceConfiguration.h"

namespace impl_ns = holder::test;

namespace
{
	holder::StarterRegistrationHelper<impl_ns::TestStarter>
		g_starterRegistration("/starters/TestStarter");
}

impl_ns::TestStarter::TestStarter(const holder::service::IStarterArgument& starterArgument)
{
	AddService(std::make_shared<service::BaseServiceConfiguration>("/root/services/ConsoleService",
					"/services/ConsoleTextService", 
					"threadA", true));

	AddService(std::make_shared<service::BaseServiceConfiguration>("/root/services/TestAlphaService",
		"/services/TestAlphaService",
		"threadA", true));
}

	