#pragma once

#include "RegistrationHelper.h"
#include "AppObjectFactoryImpl.h"
#include "IStarter.h"
#include "IService.h"

namespace holder
{
	template<typename S>
	using ServiceRegistrationHelper
		= holder::lib::RegistrationHelper<holder::base::DefaultAppObjectFactory<S, service::IService> >;

	template<typename S>
	using StarterRegistrationHelper
		= holder::lib::RegistrationHelper<holder::base::DefaultAppObjectFactory<S, service::IServiceStarter> >;
}