#include "SingletonConfig.h"
#include <cstring>

std::chrono::microseconds holder::base::SingletonConfig::GetDurationUS(const char* key)
{
	if (!strcmp(key, "execution_microsecond_wait"))
	{
		return std::chrono::microseconds(100);
	}

	throw UnknownSingletonKeyException();
}