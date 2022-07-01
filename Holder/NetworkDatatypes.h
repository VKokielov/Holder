#pragma once

#include <cinttypes>

namespace holder::network
{
	enum class ConnectionInterest
	{
		Interested,
		NotInterested
	};

	enum class ConnectionType
	{
		ActiveClient,   // created as a client connection
		ActiveServer,   // created from the server   
		Listening
	};

	enum class ConnectionCommand
	{
		Connect,
		Listen,
		Close,
		Destroy
	};

	enum class ConnectionState
	{
		Idle,
		Active,
		Closed
	};

	using NetworkSizeType = uint32_t;
	using ConnPropertyIDType = uint32_t;
	using ConnectionID = uint64_t;
	// The info tag comes from the filter and encodes information about a 
	// specific connection that was approved by a given filter
	using ConnectionInfoTag = uint32_t;
	
}