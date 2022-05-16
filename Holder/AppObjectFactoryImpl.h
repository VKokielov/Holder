#pragma once

#include "IAppObjectFactory.h"
#include "IService.h"

namespace holder::base
{
	template<typename Product, typename Interface>
	class DefaultAppObjectFactory : public base::ITypedAppObjectFactory<Interface>
	{
	private:
		using TypedArgs = typename Interface::Args;
		static_assert(std::is_base_of_v<Interface, Product>, "Product must be derived from Interface");
		static_assert(std::is_base_of_v<IAppArgument, TypedArgs>, "TypedArgs must be derived from IAppArgument");
	public:
		std::shared_ptr<Interface> Create(const IAppArgument& args) override
		{
			return std::shared_ptr<Product>(new Product(static_cast<const TypedArgs&>(args)));
		}
	};
}