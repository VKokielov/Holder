#include "ConsoleTextService.h"

#include "ServiceRegistrationHelper.h"

namespace impl_ns = holder::stream;

holder::service::ServiceRegistrationHelper<impl_ns::ConsoleTextService>
	g_serviceRegistration("/services/ConsoleTextService");

std::shared_ptr < holder::service::IServiceLink > impl_ns::ConsoleTextService::CreateProxy(const std::shared_ptr<holder::messages::IMessageDispatcher>& pReceiver)
{
	return CreateProxy_(pReceiver);
}

impl_ns::ConsoleTextService::ConsoleTextService(const holder::service::IServiceConfiguration& config)
	:ServiceBase(config),
	SOBase(std::enable_shared_from_this<ServiceBase>::shared_from_this() )
{

}