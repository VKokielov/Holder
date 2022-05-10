#pragma once

#include <cinttypes>

namespace holder::base
{
	using TypeID = uint64_t;
	constexpr TypeID TYPEID_UNTYPED = 0;
	constexpr TypeID TYPEID_FIRST_TYPED = 1;
}