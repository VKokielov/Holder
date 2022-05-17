#include "ConsoleTextService.h"

#include "RegistrationHelperAliases.h"

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


void impl_ns::ConsoleTextProxy::OutputString(const char* pString)
{
	service::SendMessage<ConsoleTextService::ConsoleTextMessage>(BaseProxy::CntPart(), pString);
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
	ServiceBase::OnCreated();
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
