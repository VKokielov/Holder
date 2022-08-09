#pragma once

#include "IAppObjectFactory.h"
#include "IService.h"

namespace holder::base
{
	template<typename Product, typename Interface,
		typename Unpacker = typename Product::Unpacker>
	class DefaultAppObjectFactory : public base::ITypedAppObjectFactory<Interface>
	{
	private:
		static_assert(std::is_base_of_v<Interface, Product>, "Product must be derived from Interface");
		static_assert(std::is_base_of_v<data::IDatum, TypedArgs>, "TypedArgs must be derived from IAppArgument");
	public:
		std::shared_ptr<Interface> Create(const data::IDatum& args) override
		{
			// Unpack		
			auto argTuple = Unpacker::Unpack(args);
			auto createLambda = [](auto&& ... args)
			{
				return new Product(std::forward<decltype(args)>(args)...);
			};

			return std::shared_ptr<Product>(std::apply(createLambda, argTuple));
		}
	};
}