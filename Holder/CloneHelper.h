#pragma once

namespace holder::lib
{

	template<typename D>
	class CloneHelper
	{
	protected:
		D* Clone_() const
		{
			return new D(*static_cast<const D*>(this));
		}
	};


}