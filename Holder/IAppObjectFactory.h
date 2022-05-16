#pragma once

#include "IAppObject.h"

#include <type_traits>
// The factories must includ
#include <memory>

namespace holder::base
{

	class IAppObjectFactory
	{
	public:
		virtual ~IAppObjectFactory() = default;
	};

	template<typename Interface>
	class ITypedAppObjectFactory : public IAppObjectFactory
	{
	public:
		static_assert(std::is_base_of_v<IAppObject, Interface>, "Interface must be derived from IAppObject");
		virtual std::shared_ptr<Interface> Create(const IAppArgument& args) = 0;
	};


}