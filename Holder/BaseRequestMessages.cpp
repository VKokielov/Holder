#include "BaseRequestMessages.h"

namespace impl_ns = holder::reqresp;

holder::base::types::TypeTag impl_ns::RequestOutgoingMessage::GetTag() const
{
	return base::constants::GetRequestOutgoingMessageTag();
}

holder::base::types::TypeTag impl_ns::RequestIncomingMessage::GetTag() const
{
	return base::constants::GetRequestIncomingMessageTag();
}