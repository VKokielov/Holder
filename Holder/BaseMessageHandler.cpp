#include "BaseMessageHandler.h"

namespace impl_ns = holder::messages;

void impl_ns::BaseMessageHandler::CreateReceiver()
{
	if (m_hasReceiver)
	{
		return;
	}

	auto pLocalDispatcher = m_pDispatcher.lock();
	if (pLocalDispatcher)
	{
		m_myReceiverID = pLocalDispatcher->CreateReceiver(GetMyBaseHandlerSharedPtr(), 
			m_pFilter, 
			0);
		m_hasReceiver = true;
	}
}

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