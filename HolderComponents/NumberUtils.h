#pragma once

#include <cinttypes>

namespace holder::lib
{

	inline uint64_t Glue(uint32_t left, uint32_t right)
	{
		return ((uint64_t)left << 32) | right;
	}

	inline void Split(uint64_t val, uint32_t& left, uint32_t& right)
	{
		left = val >> 32;
		right = val & (((uint64_t)1 << 32) - 1);
	}

}