#pragma once

#include "IServiceObject.h"
#include "IServiceMethod.h"
#include "Messaging.h"
#include "ObjectTree.h"
#include "PathLib.h"

#include <unordered_map>
#include <shared_mutex>

namespace holder::service
{

	class SOServiceManager
	{
	private:
		struct Client_
		{
			MethodID methodID;
			messages::ReceiverID receiverID;
		};

		struct SMService_
		{
			SMServiceID serviceID;
			messages::QueueID queueID;

			SMClientID freeClientID{ 0 };
			std::unordered_map<SMClientID, Client_>  m_clientMap;
		};

		class ServiceMessageReceiver : public messages::IMessageListener
		{
		public:
			ServiceMessageReceiver(const std::shared_ptr<IService>& pService);

			void OnReceiverReady(messages::DispatchID dispatchId) override;
			void OnMessage(const std::shared_ptr<messages::IMessage>& pMsg,
				messages::DispatchID dispatchId) override;

		private:
			// There is no need for a weak_ptr here.  Proxies going out of scope mean
			// that the corresponding client connection is dead.  What does a service going out of scope mean?
			// Let the service go out of scope when it will.
			std::shared_ptr<IService> m_pService;
		};

	public:
		ServiceMgrResult AddMethod(const std::shared_ptr<ISOServiceMethod>& pMethod);

		ServiceMgrResult AddService(const char* pName,
			messages::QueueID serviceQueue,
			const std::shared_ptr<IService>& pService,
			SMServiceID& serviceID);

		ServiceMgrResult FindService(const char* pName,
			SMServiceID& serviceID) const;

		ServiceMgrResult PublishService(const char* pMethodName,
			const char* pRemoteName,
			SMServiceID serviceID);

		ServiceMgrResult SendMessage(SMServiceID serviceID,
			SMClientID clientID,
			const std::shared_ptr<ISOClientMessage>& pClientMessage) const;

		ServiceMgrResult DispatchMessage(SMServiceID serviceID,
			SMClientID clientID,
			const std::shared_ptr<ISOServiceMessage>& pServiceMessage) const;

		static SOServiceManager& GetInstance();

	private:
		static thread_local lib::LockState m_lockState;

		std::shared_mutex m_mutex;
		std::vector<std::shared_ptr<ISOServiceMethod> > m_methods;
		std::unordered_map<std::string, MethodID> m_methodMap;

		std::unordered_map<SMServiceID, SMService_> m_serviceMap;
		SMServiceID m_freeServiceID{ 0 };

		// Object tree
		lib::ObjectTree<SMServiceID> m_serviceTree;
	};

}