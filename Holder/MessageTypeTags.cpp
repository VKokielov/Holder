#include "MessageTypeTags.h"

namespace impl_ns = holder::base::constants;
namespace types = holder::base::types;

types::TypeTag impl_ns::GetDestroyMessageTag()
{
	static types::TypeTag tag{ types::TypeTagManager::GetType("/messages/core/DestroyMessage") };
	return tag;
}

types::TypeTag impl_ns::GetServiceMessageTag()
{
	static types::TypeTag tag{ types::TypeTagManager::GetType("/messages/core/ServiceMessage") };
	return tag;
}

types::TypeTag impl_ns::GetOutgoingRequestTag()
{
	static types::TypeTag tag{ types::TypeTagManager::GetType("/messages/core/OutgoingRequest") };
	return tag;
}

types::TypeTag impl_ns::GetIncomingRequestTag()
{
	static types::TypeTag tag{ types::TypeTagManager::GetType("/messages/core/IncomingRequest") };
	return tag;
}

types::TypeTag impl_ns::GetCreateProxyMessageTag()
{
	static types::TypeTag tag{ types::TypeTagManager::GetType("/messages/core/CreateProxyMessage") };
	return tag;
}

types::TypeTag impl_ns::GetRequestOutgoingMessageTag()
{
	static types::TypeTag tag{ types::TypeTagManager::GetType("/messages/core/RequestOutgoingMessage") };
	return tag;
}

types::TypeTag impl_ns::GetRequestIncomingMessageTag()
{
	static types::TypeTag tag{ types::TypeTagManager::GetType("/messages/core/RequestIncomingMessage") };
	return tag;
}