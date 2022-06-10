#pragma once

#include "IAppObject.h"
#include "Messaging.h"
#include "IRequestResponse.h"
#include "IPublishSubscribe.h"
#include "IConnectionHandler.h"
#include "FlatPropertyTable.h"

#include <cinttypes>
#include <string>

namespace holder::network
{
	enum class ConnectionInterest
	{
		Interested,
		NotInterested
	};

	enum class ConnectionType
	{
		Active,
		Listening
	};

	enum class ConnectionCommand
	{
		Connect,
		Listen,
		Close
	};

	enum class ConnectionState
	{
		Idle,
		Active,
		Closed
	};

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
		// Connection creation and subscription
		// The subId below identifies the subscription ID for the connection

		// Note that the first function is also called when a listening connection
		// gets a new client.

		virtual ConnectionInterest
			OnActiveConnectionCreated(ConnectionID connId,
				pubsub::UserSubscriptionID subId,
				const ActiveConnArgs& connArgs) = 0;
		virtual ConnectionInterest
			OnListeningConnectionCreated(ConnectionID connId,
				pubsub::UserSubscriptionID subId,
				const ListeningConnArgs& connArgs) = 0;

		// Connection state
		// No need to supply the previous state as each state has
		// at most one predecessor in the state graph
		virtual void OnStateChange(ConnectionID connId,
			ConnectionState newState) = 0;

		// Active connection messages (ID is in the message)
		virtual void OnMessage(const std::shared_ptr<INetworkMessage>& pMsg) = 0;
	};

	class INetworkStatusListener : public base::IAppObject
	{
	public:
		virtual void OnRequestStateChange(reqresp::RequestID reqId,
			reqresp::RequestState newState) = 0;
	};

	class INetworkServiceProxy : public reqresp::IRequestIssuer,
		pubsub::IPublisher
	{
	public:
		// Translate a flat property table into an internal address object
		virtual IAddress* CreateAddress(const base::IFlatPropertyTable& propTable) = 0;

		// Subscribe to requests
		virtual pubsub::ServiceSubscriptionID
			SubscribeToRequestStates(std::shared_ptr<INetworkStatusListener> pStatusListener,
				pubsub::UserSubscriptionID userSubId) = 0;

		// Subscribe to connection names based on regexes
		virtual pubsub::ServiceSubscriptionID
			SubscribeToConnections(const char* pNameRegex,
				std::shared_ptr<INetworkListener> pListener,
				pubsub::UserSubscriptionID userSubId) = 0;

		// Three basic types of requests: 1) Create a connection; 2) Issue a connection command
		//  3) Send a message.
		// (3) is not a true request, to avoid latency for useless information
		virtual reqresp::RequestID
			CreateActiveConnection(const ActiveConnArgs& connArgs) = 0;
		virtual reqresp::RequestID
			CreateListeningConnection(const ListeningConnArgs& connArgs) = 0;
		virtual reqresp::RequestID
			IssueCommand(ConnectionID connId, ConnectionCommand command) = 0;

		// Connection ID is in the message
		virtual void
			SendMessage(const std::shared_ptr<INetworkMessage>& pMessage) = 0;
	};
}