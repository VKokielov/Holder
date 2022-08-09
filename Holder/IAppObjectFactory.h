#pragma once

#include "IAppObject.h"
#include "IDataTree.h"

#include <type_traits>
#include <memory>

namespace holder::base
{

	// This exception is raised when a typed object factory cannot unpack its arguments
	// from an IDatum
	class UnpackArgumentsException { };

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
		virtual std::shared_ptr<Interface> Create(const data::IDatum& args) = 0;
	};


}