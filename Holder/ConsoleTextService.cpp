#include "ConsoleTextService.h"

#include "RegistrationHelperAliases.h"

#include <iostream>

namespace impl_ns = holder::stream;

holder::ServiceRegistrationHelper<impl_ns::ConsoleTextService>
	g_serviceRegistration("/services/ConsoleTextService");

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
	:ServiceBase(config),
	SOBase(std::enable_shared_from_this<ServiceBase>::shared_from_this() )
{
	// This service has no dependencies
	ServiceBase::OnDependenciesAdded();
}