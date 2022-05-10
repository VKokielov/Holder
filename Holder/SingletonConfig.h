#pragma once

#include "SingletonConfigBase.h"

namespace holder::base
{
	class UnknownSingletonKeyException { };

	class SingletonConfig : public SingletonConfigBase<SingletonConfig>
	{
	public:
		uint64_t GetUnsignedInt(const char* key);
		int64_t GetSignedInt(const char* key);
		const std::string GetString(const char* key);
		std::chrono::microseconds GetDurationUS(const char* key);
		std::chrono::milliseconds GetDurationMS(const char* key);
	};
}