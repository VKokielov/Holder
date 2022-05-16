#include "TestServiceAlpha.h"

namespace impl_ns = holder::test;

impl_ns::TestServiceAlpha::TestServiceAlpha(const holder::service::IServiceConfiguration& config)
	:ServiceBase(config)
{
	// Add dependency to console service
	m_didTextService = ServiceBase::AddDependency("/root/ConsoleTextService");

	ServiceBase::OnDependenciesAdded();
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