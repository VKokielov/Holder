#pragma once

#include "IServiceObject.h"
#include "IServiceMethod.h"
#include "QueueManager.h"
#include "ObjectTree.h"
#include "PathLib.h"
#include "PathFromString.h"

#include "RWLockWrapper.h"

#include <unordered_map>

namespace holder::service
{

	struct CMServiceArgs
	{
		MethodID methodID;
		std::string serviceName;
		std::string serviceProxyType;
	};

	class ClientCreateException { };


	class SOClientManager
	{
	private:
		struct Client_
		{
			CMServiceID serviceID;
			CMClientID clientID;

			// A client represents a proxy object
			messages::QueueID queueID;
			messages::ReceiverID receiverID;

			Client_() = default;
		};

		struct CMService_
		{
			CMServiceArgs args;
			std::unordered_map<CMClientID, Client_> clientMap;
			CMClientID nextClientID{ 0 };

			CMService_(const CMServiceArgs& args_)
				:args(args)
			{ }
		};

		class ProxyMessageReceiver : public messages::IMessageListener
		{
		public:
			ProxyMessageReceiver(const std::shared_ptr<IProxy>& pProxy);

			void OnReceiverReady(messages::DispatchID dispatchId) override;
			void OnMessage(const std::shared_ptr<messages::IMessage>& pMsg, 
				messages::DispatchID dispatchId) override;

		private:
			std::weak_ptr<IProxy> m_pProxy;
		};

	public:

		// Functions for outside use
		ClientMgrResult AddMethod(const std::shared_ptr<ISOClientMethod>& pMethod);

		ClientMgrResult CreateClient(const char* pServiceName,
			const char* pRemoteName,
			const char* pMethodPreference,
			messages::QueueID clientQueue,
			std::shared_ptr<IProxy>& ppProxy);

		// Functions for proxy use
		ClientMgrResult SendMessage(CMServiceID serviceID,
			CMClientID clientID,
			std::shared_ptr<ISOServiceMessage>& pServiceMessage) const;
		ClientMgrResult OnProxyDestroyed(CMServiceID serviceID,
			CMClientID clientID);

		// Functions for method use
		ClientMgrResult FindService(const char* pServiceName,
			CMServiceID& serviceID) const;
		ClientMgrResult AddService(const CMServiceArgs& args,
			CMServiceID& serviceID);
		ClientMgrResult RemoveService(CMServiceID serviceID);

		ClientMgrResult DispatchMessageToProxy(CMServiceID serviceID,
			CMClientID clientID,
			std::shared_ptr<ISOClientMessage>& pClientMessage) const;

		static SOClientManager& GetInstance();

	private:

		// Used to prevent relocking when calling from outside
		static thread_local lib::LockState m_lockState;

		SOClientManager() = default;

		std::shared_mutex m_mutex;
		std::vector<std::shared_ptr<ISOClientMethod> > m_methods;
		std::unordered_map<CMServiceID, CMService_> m_serviceMap;
		CMServiceID m_freeServiceID{ 0 };

		// Object tree
		lib::ObjectTree<CMServiceID> m_serviceTree;

	};
}