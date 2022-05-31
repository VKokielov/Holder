#include "ConsoleTextService.h"

#include "RegistrationHelperAliases.h"
#include "MessageLib.h"

#include <iostream>

namespace impl_ns = holder::stream;

namespace
{
	holder::ServiceRegistrationHelper<impl_ns::ConsoleTextService>
		g_serviceRegistration("/services/ConsoleTextService");
}

std::shared_ptr<holder::messages::BaseMessageHandler> impl_ns::ConsoleTextProxy::GetMyBaseHandlerSharedPtr() 
{
	return shared_from_this();
}

void impl_ns::ConsoleTextService::ConsoleTextMessage::TypedAct(ConsoleTextClient& client)
{
	// No state information, so no need to talk to the client object or to the service
	std::cout << m_text;
}

holder::service::MessageDispatch<impl_ns::ConsoleTextProxy>& impl_ns::ConsoleTextProxy::GetMessageDispatchTable()
{
	static holder::service::MessageDispatch<impl_ns::ConsoleTextProxy> dispatch;
	return dispatch;
}

void impl_ns::ConsoleTextProxy::OnMessage(const std::shared_ptr<holder::messages::IMessage>& pMsg, 
	holder::messages::DispatchID dispatchID)
{
	BaseMessageDispatch::DispatchMessage<ConsoleTextProxy>(pMsg, dispatchID);
}


void impl_ns::ConsoleTextProxy::OutputString(const char* pString)
{
	messages::SendMessage<ConsoleTextService::ConsoleTextMessage>(BaseProxy::CntPart(), pString);
}

// ConsoleTextService

holder::service::MessageDispatch<impl_ns::ConsoleTextService>& impl_ns::ConsoleTextService::GetMessageDispatchTable()
{
	static holder::service::MessageDispatch<impl_ns::ConsoleTextService> dispatch;
	return dispatch;
}

std::shared_ptr < holder::service::IServiceLink > impl_ns::ConsoleTextService::CreateProxy(const std::shared_ptr<holder::messages::IMessageDispatcher>& pReceiver)
{
	return CreateProxy_(pReceiver);
}

impl_ns::ConsoleTextService::ConsoleTextService(const holder::service::IServiceConfiguration& config)
	:ServiceBase(config)
{
	ServiceBase::SetAutoCompleteStartup(true);
}

void impl_ns::ConsoleTextService::OnCreated()
{
	SOBase::SetDispatcher(GetMyMessageDispatcherSharedPtr());
	SOBase::SetThreadName(ServiceBase::GetExecutionThreadName());

	ServiceBase::OnCreated();
}



void impl_ns::ConsoleTextService::OnMessage(const std::shared_ptr<holder::messages::IMessage>& pMsg,
	holder::messages::DispatchID dispatchID)
{
	BaseMessageDispatch::DispatchMessage<ConsoleTextService>(pMsg, dispatchID);
}
std::shared_ptr<holder::messages::MessageDequeDispatcher> impl_ns::ConsoleTextService::GetMyMessageDispatcherSharedPtr()
{
	return shared_from_this();
}
std::shared_ptr<holder::messages::IMessageListener> impl_ns::ConsoleTextService::GetMyListenerSharedPtr()
{
	return shared_from_this();
}
std::shared_ptr<holder::base::startup::ITaskStateListener> 
impl_ns::ConsoleTextService::GetMyTaskStateListenerSharedPtr()
{
	return shared_from_this();
}
