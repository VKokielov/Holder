#include "BaseProxy.h"
#include "MessageLib.h"

namespace impl_ns = holder::service;
namespace messages = holder::messages;

impl_ns::BaseProxy::BaseProxy(const std::shared_ptr<messages::IMessageDispatcher>& pDispatcher,
	std::shared_ptr<messages::ISenderEndpoint> pClientEndpoint)
	:BaseMessageHandler(pDispatcher),
	m_pCounterpart(pClientEndpoint)
{

}

void impl_ns::BaseProxy::OnServiceMessage(messages::IMessage& rMsg,
	messages::DispatchID dispatchId)
{
	// Act on the proxy
	static_cast<IServiceMessage&>(rMsg).Act(*this);
}

holder::messages::ReceiverID impl_ns::BaseProxy::GetReceiverID() const
{
	return BaseMessageHandler::GetReceiverID_();
}

impl_ns::BaseProxy::~BaseProxy()
{
	// Send a "destroy" message for this client
	messages::SendMessage < DestroyClientMessage>(m_pCounterpart);
}

