#include "TestServiceAlpha.h"
#include "RegistrationHelperAliases.h"

#include <sstream>

namespace impl_ns = holder::test;

namespace
{
	holder::ServiceRegistrationHelper<impl_ns::TestServiceAlpha>
		g_serviceRegistration("/services/TestServiceAlpha");
}

impl_ns::TestServiceAlpha::TestServiceAlpha(const holder::service::IServiceConfiguration& config)
	:ServiceBase(config)
{

}

void impl_ns::TestServiceAlpha::OnCreated()
{
	ServiceBase::AddService();

	// Add dependency to console service
	m_didTextService = ServiceBase::AddDependency("/root/services/ConsoleService");

	ServiceBase::OnDependenciesAdded();
}

void impl_ns::TestServiceAlpha::TextSender::OnTimer(holder::base::TimerID timerId)
{
	std::stringstream ssm;
	ssm << "Counter value: " << m_counter << '\n';
	++m_counter;

	std::string sText(ssm.str());
	m_pTextProxy->OutputString(sText.c_str());
}

bool impl_ns::TestServiceAlpha::Init()
{
	// Get the dependency
	auto pCTS = ServiceBase::GetDependencyPtr(m_didTextService);
	if (!pCTS)
	{
		// Failed.  
		MarkFailed();
		return false;
	}

	base::LockedSharedObjectRef<stream::ITextService> serviceRef(*pCTS);

	auto pProxy = 
		std::dynamic_pointer_cast<stream::ITextServiceProxy> 
		 (serviceRef->CreateProxy(ServiceBase::GetMySharedPtr()));

	if (!pProxy)
	{
		MarkFailed();
		return false;
	}

	// Create a stimulator
	auto pStimulator = std::make_shared<TextSender>(pProxy);

	// Submit the stimulator to the execution manager
	base::ExecutionManager::GetInstance().SetTimer("/service/TestServiceAlpha/TestTimer",
		500 * 1000, true, 10, pStimulator);

	// Mark as ready
	return ServiceBase::Init();
}