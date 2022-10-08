#pragma once

#include "Messaging.h"
#include "BaseMessageHandler.h"

#include <cinttypes>

namespace holder::service
{

	using CMClientID = uint32_t;
	using CMServiceID = uint64_t;

	using SMClientID = uint32_t;
	using SMServiceID = uint64_t;

	class ISOMessage;
	class ISOClientMessage;
	class ISOServiceMessage;

#ifdef _DEBUG
	class MessageCastException { };
#endif

	enum class ClientMgrResult
	{
		OK,
		CantAddDuplicate,
		CantConnectService,
		ServiceProxyFactoryNotFound,
		ServiceProxyFactoryTypeError,
		ServiceProxyNotCreated,
		ServiceNotFound,
		ClientNotFound,
		CantSendMessageToClient,
		CantAddServiceName
	};

	enum class ServiceMgrResult
	{
		OK,
		CantAddDuplicate,
		CantAddServiceName
	};

	class IServiceObject : public base::IAppObject
	{

	};

	class IProxy : public IServiceObject
	{
	public:
		virtual void OnClientMessage(const ISOClientMessage& message) = 0;
	};

	class IService : public IServiceObject
	{
	public:
		virtual const char* GetProxyTypeName() const = 0;
		virtual void OnNewClient(SMClientID clientID) = 0;
		virtual void OnClientRemoved(SMClientID clientID) = 0;
		virtual void OnServiceMessage(const ISOServiceMessage& message, 
			SMClientID clientID) = 0;
	};

	class ISOClientMessage : public messages::IMessage
	{

	};

	class ISOServiceMessage : public messages::IMessage
	{

	};
}