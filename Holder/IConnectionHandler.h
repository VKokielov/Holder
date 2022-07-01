#pragma once

#include "IAppObject.h"
#include "Messaging.h"
#include "NetworkDatatypes.h"

#include <cinttypes>
#include <memory>

namespace holder::network
{
	class IAddress;


	class INetworkMessage : public messages::IMessage
	{
	public:
		virtual ConnectionID GetConnectionID() const = 0;
	};

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
		virtual bool Encode(const std::shared_ptr<INetworkMessage>& pMessage,
			IDataEndpoint& dataEndpoint) = 0;
		virtual bool EncodeHeartbeat(IDataEndpoint& dataEndpoint) = 0;
		virtual bool Decode(const uint8_t* pData, 
			NetworkSizeType size,
			messages::ISenderEndpoint& senderEndpoint,
			bool& heartbeatReceived) = 0;
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

	class ISubscriptionFilter : public base::IAppObject
	{
	public:
		// Filter connections to decide which match a given subscription
		// The tag is ignored unless the connection is subscribed; if it is,
		// the tag is sent to the subscriber as additional information about
		// the connection
		virtual bool ShouldSubscribeToConnection(const char* pConnectionName,
			ConnectionType connectionType,
			const IAddress* pAddress,
			ConnectionInfoTag& tag) = 0;
	};

}