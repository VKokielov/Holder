#pragma once

#include "TypeInterface.h"

namespace holder::base
{

	class IAppObject
	{
	public:
		virtual ~IAppObject() = default;
	};

	class IAppObjectInitializer
	{
	public:
		virtual ~IAppObjectInitializer() = default;
	};

	class IAppObjectFactory : public base::IAppObject
	{
	public:
		virtual TypeID GetProductType() const = 0;
		virtual IAppObject* Create(const IAppObjectInitializer&) const = 0;
		virtual ~IAppObjectFactory() = default;
	};

}