#pragma once

namespace holder::lib
{
#ifdef HAS_RTTI
	template<typename T, typename U>
	T* best_cast(U* pU)
	{
		return dynamic_cast<T*>(pU);
	}

	template<typename T, typename U>
	T& best_cast(U& rU)
	{
		return dynamic_cast<T&>(rU);
	}
#else
	template<typename T, typename U>
	T* best_cast(U* pU)
	{
		return static_cast<T*>(pU);
	}

	template<typename T, typename U>
	T& best_cast(U& rU)
	{
		return static_cast<T&>(rU);
	}

#endif

}