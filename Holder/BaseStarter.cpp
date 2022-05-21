#include "BaseStarter.h"
#include "SharedObjects.h"
#include "StartupTaskManager.h"
#include "ExecutionManager.h"
#include "AppLibrary.h"
#include "PathLib.h"
#include "IService.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace impl_ns = holder::service;

holder::lib::PathFromString impl_ns::BaseStarter::m_servicePrefix("/root/services");

bool impl_ns::BaseStarter::Start()
{
	if (m_startCallSucceeded)
	{
		return true;
	}

	// In case this is not the first try...
	m_serviceMap.clear();

	// Create all the services and begin the executor of the startup manager
	for (const auto& serviceStartDesc : m_serviceStartMap)
	{
		// Create the service and add it to the store
		ServiceDesc_ serviceDesc;

		// Need to use a lambda because the object will be created as shared
		auto facMakeService = [&serviceStartDesc]()
		{
			return serviceStartDesc.second.pServiceFactory->Create(*serviceStartDesc.second.pServiceConfig);
		};
		
		auto initService = [](const std::shared_ptr<IService>& pService)
		{
			pService->OnCreated();
		};

		auto pService = base::MakeInitializedSharedObjectWithFactory<service::IService>(facMakeService, initService);

		// Add to the object store
		// This should not fail due to name clashes...
		serviceDesc.serviceStoredToken = base::SharedObjectStore::GetInstance().AddObject(serviceStartDesc.second.servicePath.c_str(), pService);
		serviceDesc.pService = pService;

		m_serviceMap.emplace(serviceStartDesc.second.servicePath, serviceDesc);

		
	}

	// Start the startup task manager, making sure to end it when all startup tasks are complete
	base::startup::StartupTaskManager::GetInstance().StartExecutor(true);
	m_startCallSucceeded = true;

	return true;
}

bool impl_ns::BaseStarter::Run()
{
	if (!m_startCallSucceeded)
	{
		// Call Start before Run
		return false;
	}

#ifdef _WIN32
	WinInstallInterruptHandler();
#endif

	std::chrono::microseconds wakeupTime(100 * 1000);
	// RunLoop continues to run until all threads terminate or termination is requested
	base::ExecutionManager::GetInstance().RunLoop(wakeupTime);

	return true;
}

impl_ns::AddServiceResult impl_ns::BaseStarter::AddService(const std::shared_ptr<IServiceConfiguration>& pServiceConfiguration)
{
	std::string servicePath(pServiceConfiguration->GetServicePath());
	std::string serviceTypeName(pServiceConfiguration->GetServiceTypeName());

	lib::PathFromString analyzedPath(servicePath.c_str());
	if (!lib::PathStartsWith(analyzedPath, m_servicePrefix))
	{
		return AddServiceResult::ServiceNotInRoot;
	}

	if (m_serviceMap.count(servicePath) > 0)
	{
		return AddServiceResult::DuplicateServiceName;
	}

	std::shared_ptr<base::IAppObjectFactory> pServiceFactoryRaw;

	if (!base::AppLibrary::GetInstance().FindFactory(serviceTypeName.c_str(), pServiceFactoryRaw))
	{
		return AddServiceResult::MissingServiceFactory;
	}

	auto pServiceFactory
		= std::dynamic_pointer_cast<base::ITypedAppObjectFactory<service::IService>>(pServiceFactoryRaw);

	if (!pServiceFactory)
	{
		return AddServiceResult::ServiceFactoryTypeError;
	}

	ServiceStartDesc_ serviceStartDesc;
	serviceStartDesc.servicePath = servicePath;
	serviceStartDesc.serviceTypeName = serviceTypeName;
	serviceStartDesc.pServiceFactory = pServiceFactory;
	serviceStartDesc.pServiceConfig = pServiceConfiguration;

	m_serviceStartMap.emplace(servicePath, serviceStartDesc);

	return AddServiceResult::OK;
}

#ifdef _WIN32
namespace
{

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		// Lock up shop and request termination from all threads
		holder::base::ExecutionManager::GetInstance().LockShop(true);
	}

	return FALSE;
}
}

void impl_ns::BaseStarter::WinInstallInterruptHandler()
{
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
}

#endif