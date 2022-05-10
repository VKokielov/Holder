#include "BaseProxy.h"

namespace impl_ns = holder::service;
namespace messages = holder::messages;

impl_ns::BaseProxy::BaseProxy(const std::shared_ptr<messages::IMessageDispatcher>& pDispatcher,
	std::shared_ptr<messages::ISenderEndpoint> pClientEndpoint)
	:BaseMessageHandler(pDispatcher,  std::make_shared<ServiceMessageFilter>()),
	m_pCounterpart(pClientEndpoint)
{

}

void impl_ns::BaseProxy::OnMessage(const std::shared_ptr<messages::IMessage>& pMsg,
	messages::DispatchID dispatchId)
{
	// Act on the proxy
	static_cast<IServiceMessage&>(*pMsg).Act(*this);
}

holder::messages::ReceiverID impl_ns::BaseProxy::GetReceiverID() const
{
	return BaseMessageHandler::GetReceiverID_();
}

impl_ns::BaseProxy::~BaseProxy()
{
	// Send a "destroy" message for this client
	SendMessage < DestroyClientMessage>(m_pCounterpart);
}
