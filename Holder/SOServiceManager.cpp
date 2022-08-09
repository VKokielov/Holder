#include "SOServiceManager.h"

namespace impl = holder::service;

impl::SOServiceManager::
ServiceMessageReceiver::ServiceMessageReceiver(const std::shared_ptr<IService>& pService)
	:m_pService(pService)
{

}

void impl::SOServiceManager::ServiceMessageReceiver::OnReceiverReady(messages::DispatchID dispatchId)
{

}
void impl::SOServiceManager::ServiceMessageReceiver::OnMessage(const std::shared_ptr<messages::IMessage>& pMsg,
	messages::DispatchID dispatchId)
{
	// dispatchID is also clientID
	ISOServiceMessage* pRawMessage = nullptr;
#ifdef _DEBUG
	// RTTI!
	pRawMessage = dynamic_cast<ISOServiceMessage*>(pMsg.get());

	if (!pRawMessage)
	{
		throw MessageCastException();
	}
#else
	pRawMessage = static_cast<ISOClientMessage*>(pMsg.get());
#endif

	m_pService->OnServiceMessage(*pRawMessage, (SMClientID)dispatchId);
}

