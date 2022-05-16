#pragma once

#include "BaseStarter.h"

namespace holder::test
{
	class TestStarter : public service::BaseStarter
	{
	public:
		TestStarter(const service::IStarterArgument& starterArgument);

	};
}