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
			std::string serviceQueueName;
			std::shared_ptr<base::ITypedAppObjectFactory<IService> >
				pServiceFactory;
			std::shared_ptr<data::IDatum>
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

		AddServiceResult AddService(const std::shared_ptr<data::IDatum>& pServiceConfiguration);

		messages::QueueID AddQueue(const char* pQueueName, 
			std::shared_ptr<messages::IMessageDispatcher> pDispatcher);

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