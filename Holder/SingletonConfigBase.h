#pragma once

#include <cinttypes>
#include <string>
#include <chrono>
#include <type_traits>

namespace holder::base
{
	/*
		This is a base class used for singleton configurations, which may happen
		before any code in main() executes and in unpredictable order.  Clearly the
		values here should be constants.

		It defines the interface and all basic behavior, including GetInstance().  
		Deriving classes need only implement the functions in question
	*/

	class UndefinedConfigUsedException { };

	template<typename D>
	class SingletonConfigBase
	{
	public:
		uint64_t GetUnsignedInt(const char* key)
		{
			throw UndefinedConfigUsedException();
		}

		int64_t GetSignedInt(const char* key)
		{
			throw UndefinedConfigUsedException();
		}

		const std::string GetString(const char* key)
		{
			throw UndefinedConfigUsedException();
		}

		std::chrono::microseconds GetDurationUS(const char* key)
		{
			throw UndefinedConfigUsedException();
		}

		std::chrono::milliseconds GetDurationMS(const char* key)
		{
			throw UndefinedConfigUsedException();
		}

		static D& GetInstance()
		{
			static D me;
			return me;
		}

	protected:
		SingletonConfigBase() { }
	};


}