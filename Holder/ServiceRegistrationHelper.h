#pragma once

#include "RegistrationHelper.h"
#include "AppObjectFactoryImpl.h"
#include "IService.h"

namespace holder::service
{
	template<typename S>
	using ServiceRegistrationHelper
		= holder::lib::RegistrationHelper<holder::base::DefaultAppObjectFactory<S, service::IService> >;

}