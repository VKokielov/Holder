#pragma once

#include "IServiceObject.h"

namespace holder::service
{
	using MethodID = size_t;

	class ISOClientMethod : public base::IAppObject
	{
	public:
		virtual const char* GetMethodName() const = 0;
		virtual void OnAdded(MethodID methodID) = 0;

		// Connect to a service.  After a successful call, the SO client manager
		// should have an entry to the service in its registry
		virtual ClientMgrResult ConnectService(const char* pServiceName,
			CMServiceID& serviceID) = 0;

		virtual ClientMgrResult SendMessage(CMServiceID serviceID,
			CMClientID clientID,
			std::shared_ptr<ISOServiceMessage>& pServiceMessage) = 0;
		
		virtual ClientMgrResult OnNewClient(CMServiceID serviceID,
			CMClientID clientID) = 0;

		virtual ClientMgrResult OnClientDestroyed(CMServiceID serviceID,
			CMClientID clientID) = 0;
	};

	class ISOServiceMethod : public base::IAppObject
	{
	public:
		virtual const char* GetMethodName() const = 0;
		virtual void OnAdded(MethodID methodID) = 0;

		virtual ServiceMgrResult PublishService(const char* pPublishedServiceName,
			SMServiceID serviceID,
			const std::shared_ptr<IService>& pService) = 0;

		virtual ServiceMgrResult SendMessage(SMServiceID serviceID,
			SMClientID clientID,
			const std::shared_ptr<ISOClientMessage>& pClientMessage) = 0;

	};

}