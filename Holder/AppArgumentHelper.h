#pragma once

#include "IAppObject.h"

#include <type_traits>
namespace holder::base
{

	template<typename D, typename B>
	class AppArgumentHelper : public B
	{
	public:
		static_assert(std::is_base_of_v<IAppArgument, B>, "Base must derive from IAppArgument");

		IAppArgument* Clone() const override
		{
			return new D(static_cast<const D&>(*this));
		}
	};

}