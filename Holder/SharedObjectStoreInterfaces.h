#pragma once

#include <cinttypes>

namespace holder::base
{
	struct StoredObjectToken
	{
		uint32_t token;
	};

	constexpr StoredObjectToken g_invalidObjToken{ 0 };

	using StoredObjectID = uint32_t;

	class ObjectStoreException { };
}