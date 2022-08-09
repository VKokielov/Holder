#pragma once

#include <memory>

#include "IAppObject.h"
#include "Messaging.h"

namespace holder::base
{

	using SharedObjectPtr = std::shared_ptr<ISharedObject>;
	using SharedObjectID = uint64_t;

	class ISharedObject : public IAppObject
	{
	};

	struct StoredObjectToken
	{
		uint32_t token;
	};

	constexpr StoredObjectToken g_invalidObjToken{ 0 };

	using StoredObjectID = uint32_t;

	class ObjectStoreException { };
}