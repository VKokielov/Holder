#pragma once

#include "IAppObject.h"

#include <type_traits>

namespace holder::base
{

	class IAppObjectFactory
	{
	public:
		virtual IAppObject* Create(const IAppArgument& args) = 0;
		virtual ~IAppObjectFactory() = default;
	};

	template<typename Interface>
	class ITypedAppObjectFactory : public IAppObjectFactory
	{
	public:
		static_assert(std::is_base_of_v<IAppObject, Interface>, "Interface must be derived from IAppObject");
		virtual Interface* Create(const IAppArgument& args) = 0;
	};


}