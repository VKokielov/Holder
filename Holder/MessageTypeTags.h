#pragma once

#include "TypeTags.h"

namespace holder::base::constants
{
	// Declarations of common type tags for messages
	
	types::TypeTag GetDestroyMessageTag();

	types::TypeTag GetServiceMessageTag();

	types::TypeTag GetOutgoingRequestTag();

	types::TypeTag GetIncomingRequestTag();

	types::TypeTag GetCreateProxyMessageTag();

	types::TypeTag GetRequestOutgoingMessageTag();

	types::TypeTag GetRequestIncomingMessageTag();

	types::TypeTag GetSCommClientMessageTag();

	types::TypeTag GetSCommServiceMessageTag();

}