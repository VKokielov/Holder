#pragma once

#include "AppLibrary.h"
#include <type_traits>
#include <iostream>

#include <memory>

namespace holder::lib
{
	class RegistrationException { };

	template<typename Factory>
	class RegistrationHelper
	{
		static_assert(std::is_base_of_v<base::IAppObjectFactory, Factory>,
			"RegistrationHelper expects factory types");

	public:
		RegistrationHelper(const char* pTypeAddress)
		{
			base::AppLibrary& appLib = base::AppLibrary::GetInstance();
			
			auto pFactory = std::make_shared<Factory>();
			
			if (!appLib.RegisterFactory(pTypeAddress, pFactory))
			{
				std::cerr << "COULD NOT REGISTER FACTORY " << pTypeAddress << " (duplicate name?)  Aborting.\n";
				throw RegistrationException();
			}
		}

	};


}