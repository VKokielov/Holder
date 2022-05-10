#pragma once

#include "SharedObjects.h"
#include "StartupTaskManagerInterfaces.h"
#include "SharedObjectStoreInterfaces.h"
#include "IAppObjectFactory.h"

namespace holder::service
{
	// This class should hold the instance-specific configuration 
	class IServiceInstanceConfiguration : public base::IAppArgument
	{
	
	};


	class IServiceConfiguration : public base::IAppArgument
	{
	public:
		virtual const char* GetServicePath() const = 0;
		virtual const char* GetServiceTypeName() const = 0;
		virtual const char* GetServiceThreadName() const = 0;
		virtual bool TraceLostMessages() const = 0;
		virtual const IServiceInstanceConfiguration* GetInstanceConfiguration() const = 0;
	};

	class IService : public base::ISharedObject
	{
	public:
		using Args = IServiceConfiguration;
		// This should be called by the starter after all dependencies have been added
		virtual void StartService(const base::SharedObjectPtr& pMyPtr) = 0;
	};

	using ServiceFactory = base::ITypedAppObjectFactory<IService>;
}