#pragma once

#include "IAppObject.h"
#include "Messaging.h"
#include "IRequestResponse.h"
#include "IPublishSubscribe.h"

#include <cinttypes>

namespace holder::network
{
	class IAddress;
	class IConnectionProxy;

	using NetworkSizeType = uint32_t;
	using ConnPropertyIDType = uint32_t;


	enum class ConnectionType
	{
		Active,
		Listening
	};

	enum class ConnectionState
	{
		Idle,  // Object exists but no connection request has been made
		Requested,  // Connection requested
		Listening,
		ConnectFailed,
		Active,
		ClosedLocal,
		ClosedRemote
	};

	constexpr ConnPropertyIDType CONN_ADDRESS = 0;
	constexpr ConnPropertyIDType CONN_SERVICE = 1;
	constexpr ConnPropertyIDType CONN_NETWORK_PROTOCOL = 2; // tcp, udp

	class IDataEndpoint : public base::IAppObject
	{
	public:
		virtual void SendData(const uint8_t* pData, NetworkSizeType size) = 0;
	};

	// Handlers
	class IConnectionHandler : public base::IAppObject
	{

	};

	class IActiveHandler : public IConnectionHandler
	{
	public:
		virtual bool Encode(const std::shared_ptr<messages::IMessage>& pMessage,
			                IDataEndpoint& dataEndpoint) = 0;
		virtual bool Decode(const uint8_t* pData, NetworkSizeType size,
			messages::ISenderEndpoint& senderEndpoint) = 0;
	};

	class IListeningHandler : public base::IAppObject
	{
	public:
		// This allows the connection owner to filter quickly which connections
		// are acceptable.  At the price of some extra locking overhead it's possible
		// to save an enormous amount of time accepting, for example, only known
		// connections
		virtual bool ShouldAcceptConnection(const IAddress& remoteAddress) = 0;
	};

	// Listeners
	class IConnectionListener : public pubsub::ISubscriber
	{
	public:
		virtual void OnConnectionStateChange(pubsub::UserSubscriptionID subID, 
			ConnectionState oldState,
			ConnectionState newState) = 0;
	};

	class IActiveConnectionListener : public pubsub::ISubscriber
	{
	public:
		virtual void OnConnectionNewMessages(pubsub::UserSubscriptionID subID) = 0;
	};

	class IListeningConnectionListener : public pubsub::ISubscriber
	{
	public:
		virtual void OnNewActiveConnection(const std::shared_ptr<IActiveConnectionProxy>&
			pActiveProxy) = 0;
	};

	class IConnectionMessageCallback : public base::IAppObject
	{
	public:
		virtual bool OnMessage(const std::shared_ptr<messages::IMessage>& pMessage) = 0;
	};

	class IConnectionProxy : public base::IAppObject
	{
	public:
		virtual ConnectionType GetType() const = 0;
		virtual const IAddress& GetAddress() const = 0;

		// Request from the network service to connect
		// In the case of listening sockets, open/bind the socket
		// In the case of active sockets, connect
		virtual bool RequestConnect() = 0;
		virtual bool RequestDisconnect() = 0;
	};

	class IActiveConnectionProxy : public IConnectionProxy,
		public pubsub::IPublisher
	{
	public:
		// Get the sender endpoint for this connection
		virtual const std::shared_ptr<messages::ISenderEndpoint>&
			GetSenderEndpoint() const = 0;
		virtual void GetNewMessages(IConnectionMessageCallback& messageCallback)
			const = 0;

	};

	class IListeningConnectionProxy : public IConnectionProxy,
		public pubsub::IPublisher
	{

	};

	struct ConnectionDescription
	{
		const IAddress* pAddress;
		ConnectionType connType;
		std::shared_ptr<IConnectionHandler> pHandler;
		// Setting this to true means that the connection will try to connect at once
		// (if it was not connected already)
		// This may be faster but will add chaos, because the actual state of the connection
		// will not be known precisely when the object comes back.
		// Note that this state is sent in messages, so there is always some lag
		bool autoConnect;

		reqresp::RequestDescription connRequestDescription;
	};


	class INetworkServiceProxy : public base::IAppObject
	{
	public:
		virtual IAddress* CacheAddress(const char* pAddressDescription) = 0;
		// The response here is a connection proxy representing a connection
		// The message dispatcher used is the dispatcher already associated with this connection
		virtual reqresp::RequestID
			RequestConnection(const ConnectionDescription& connDesc) = 0;
	};

	class IAddress
	{
	public:
		virtual const char* GetConnectionProperty(uint32_t propertyId) const = 0;
	};



}