#pragma once

#include "IAppObject.h"
#include "Messaging.h"

#include <cinttypes>
#include <memory>

namespace holder::network
{
	class IAddress;

	using NetworkSizeType = uint32_t;
	using ConnPropertyIDType = uint32_t;
	using ConnectionID = uint64_t;

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

}