#include "BaseMessageHandler.h"

namespace impl_ns = holder::messages;


std::shared_ptr<holder::messages::ISenderEndpoint> impl_ns::BaseMessageHandler::CreateSenderEndpoint()
{
	std::shared_ptr<holder::messages::ISenderEndpoint> pRet;

	auto pDispatcher = LockDispatcher();

	if (pDispatcher)
	{
		pRet = pDispatcher->CreateEndpoint(GetReceiverID_());
	}

	return pRet;
}