#pragma once

#include "IStarter.h"
#include "IService.h"
#include "SharedObjects.h"
#include "SharedObjectStore.h"
#include "PathFromString.h"

#include <unordered_map>
#include <string>

namespace holder::service
{

	enum class AddServiceResult
	{
		OK,
		DuplicateServiceName,
		MissingServiceFactory,
		ServiceFactoryTypeError,
		ServiceNotInRoot   // The root service path is "/root/services".
	};


	class BaseStarter : public IServiceStarter
	{
	private:
		struct ServiceStartDesc_
		{
			std::string servicePath;
			std::string serviceTypeName;
			std::shared_ptr<base::ITypedAppObjectFactory<IService> >
				pServiceFactory;
			std::shared_ptr<IServiceConfiguration>
				pServiceConfig;
		};

		struct ServiceDesc_
		{
			base::SharedObjectPtr pService;
			base::StoredObjectToken serviceStoredToken;
		};
	public:
		virtual bool Start() override;
		virtual bool Run() override;
	protected:
		BaseStarter() = default;

		AddServiceResult AddService(const std::shared_ptr<IServiceConfiguration>& pServiceConfiguration);

	private:
#ifdef _WIN32
		void WinInstallInterruptHandler();
		// Declaring the handler here as a static function would require introducing a needless dependency on Windows.h
		// Instead it'll be declared in the file, in an anonymous namespace
#endif

		static lib::PathFromString m_servicePrefix;
		
		bool m_startCallSucceeded{ false };

		std::unordered_map<std::string, ServiceStartDesc_>
			m_serviceStartMap;

		std::unordered_map<std::string, ServiceDesc_> m_serviceMap;
	};


}