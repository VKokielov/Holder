#pragma once

#include <cinttypes>

namespace holder::base
{
	using TypeID = uint64_t;

	class IType
	{
	public:
		virtual ~IType() = default;
	};

}