#pragma once
#include "AppInterface.h"
#include "TypeManager.h"
#include "BestCast.h"
#include <type_traits>

namespace holder::base
{
	template<typename T>
	struct type_name_traits
	{
		static std::string GetTypeName()
		{
			return T::GetTypeName();
		}
	};

	template<typename Product, 
		typename Initializer>
	class TypedFactory : public IAppObjectFactory
	{
	private:

		static TypeDesc GetFactoryTypeDesc()
		{
			TypeDesc td;
			td.typeName = type_name_traits<Product>::GetTypeName();
		}

		static TypeID GetFactoryTypeID()
		{
			static TypeID productType = TypeManager::GetInstance().FindType(GetFactoryTypeDesc());
		}
	public:
		static_assert(std::is_base_of_v<Product, IAppObject>, "Can only produce IAppObjects");
		static_assert(std::is_base_of_v<Initializer, IAppObjectInitializer>, 
			"Initializer must derive from IAppObjectInitializer");

		Product* Create(const IAppObjectInitializer& initializer) const override
		{
			const Initializer& 

			return new Product()
		}
	};

}