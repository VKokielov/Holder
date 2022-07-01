#pragma once

#include "IAppObject.h"
#include "Messaging.h"
#include "IRequestResponse.h"
#include "IPublishSubscribe.h"
#include "IConnectionHandler.h"
#include "FlatPropertyTable.h"
#include "NetworkDatatypes.h"

#include <cinttypes>
#include <string>

namespace holder::network
{


	constexpr ConnPropertyIDType CONN_ADDRESS = 0;
	constexpr ConnPropertyIDType CONN_SERVICE = 1;
	constexpr ConnPropertyIDType CONN_NETWORK_PROTOCOL = 2; // tcp, udp

	struct ActiveConnArgs
	{
		std::string connName;
		std::shared_ptr<IActiveHandler> pHandler;
		unsigned long usIncomingHeartbeat;
		unsigned long usOutgoingHeartbeat;
		const IAddress* pAddress;
	};

	struct ListeningConnArgs
	{
		std::string connName;
		std::shared_ptr<IListeningHandler> pHandler;
		// Note: the connName in the activeTemplate is normally used as a prefix
		// for all active connections arising from this listening connection
		ActiveConnArgs activeTemplate;
		const IAddress* pAddress;
	};

	class IAddress : public base::IFlatPropertyTable
	{
	};

	class INetworkListener : public base::IAppObject
	{
	public:
		// the tag is a value sent from the subscription filter to the listener
		// wasCreated is true if the subscription was created after the filter
		// was added; if the connection was already in existence then wasCreated
		// is false.  (This may indicate lost events)
		virtual void
			OnConnectionFound(pubsub::UserSubscriptionID subId,
				ConnectionID connId,
				bool wasCreated,
				ConnectionState startState,
				ConnectionInfoTag tag) = 0;

		virtual void
			OnConnectionDestroyed(pubsub::UserSubscriptionID subId,
				ConnectionID connID) = 0;

		// Connection state
		virtual void OnStateChange(pubsub::UserSubscriptionID subId,
			ConnectionID connId,
			ConnectionState prevState,
			ConnectionState newState) = 0;

		// Active connection messages (ID is in the message)
		virtual void OnMessage(pubsub::UserSubscriptionID subId, 
			const std::shared_ptr<INetworkMessage>& pMsg) = 0;
	};

	class INetworkServiceProxy : public reqresp::IRequestIssuer,
		pubsub::IPublisher
	{
	public:
		// Translate a flat property table into an internal address object
		virtual IAddress* CreateAddress(const base::IFlatPropertyTable& propTable) = 0;

		// Subscribe to connection classes
		// A connection class is a description of connections in terms of a filter
		// The filter runs on the network service, and indicates interest in a connection
		// The 
		virtual pubsub::ServiceSubscriptionID
			SubscribeToConnectionClass(std::shared_ptr<ISubscriptionFilter> pFilter,
				std::shared_ptr<INetworkListener> pListener,
				pubsub::UserSubscriptionID userSubId) = 0;

		// Three basic types of requests: 1) Create a connection; 2) Issue a connection command
		//  3) Send a message.
		// (3) is not a true request, to avoid latency for useless information
		virtual reqresp::RequestID
			CreateActiveConnection(const ActiveConnArgs& connArgs,
				const reqresp::RequestDescription& reqDesc) = 0;
		virtual reqresp::RequestID
			CreateListeningConnection(const ListeningConnArgs& connArgs,
				const reqresp::RequestDescription& reqDesc) = 0;
		virtual reqresp::RequestID
			IssueCommand(ConnectionID connId, ConnectionCommand command,
				const reqresp::RequestDescription& reqDesc) = 0;

		// Connection ID is in the message
		virtual void
			SendMessage(const std::shared_ptr<INetworkMessage>& pMessage) = 0;
	};
}