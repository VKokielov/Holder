#pragma once

#include "IAppObject.h"
#include "Messaging.h"
#include "IService.h"
#include "IProxy.h"

#include <cinttypes>
#include <memory>

namespace holder::reqresp
{

	using RequestID = uint32_t;
	using ResponseID = uint32_t;

	enum class RequestState
	{
		Issued,
		Completed,
		Failed,
		CanceledTimeout, // Timeout; cancel sent to remote but not acknowledged
		CanceledUser, // User cancel sent but not acknowledged
		CancelAcknowledged // Cancel acknowledged by the remote
	};

	class IRequestInitializer : public base::IAppObject
	{

	};

	class IRequestInfo : public base::IAppObject
	{
	public:
		virtual const IRequestInitializer GetRequestInitializer() const = 0;
		virtual RequestState GetRequestState() const = 0;
		virtual const std::shared_ptr<base::IAppObject>
			GetRequestResult() const = 0;
	};

	class IRequestIssuer : public base::IAppObject
	{
	public:
		virtual const IRequestInfo* GetRequestInfo(RequestID requestID) const = 0;
		virtual bool CancelRequest(RequestID requestID) = 0;
		virtual bool PurgeRequest(RequestID requestID) = 0;
	};

	class IRequestHandler
	{
	public:
		virtual void CancelRequest(RequestID requestID, messages::DispatchID dispatchID) = 0;
	};
	
	template<typename RequestInitializer>
	class ITypedRequestHandler : public virtual IRequestHandler
	{
	public:
		virtual bool CreateRequest(const RequestInitializer& requestInitializer,
			RequestID requestID, messages::DispatchID clientID) = 0;
	};

	class IRequestListener : public base::IAppObject
	{
	public:
		virtual void OnRequestStateChange(RequestID reqId,
			ResponseID respId,
			RequestState oldState,
			RequestState newState) = 0;
		virtual void OnRequestPurged(RequestID reqId, ResponseID respId) = 0;
	};

	struct RequestDescription
	{
		std::shared_ptr<IRequestListener> pListener;
		ResponseID respId;
		unsigned long usTimeout{ 0 };  // 0 means no timeout
	};


	class IRequestMessage : public messages::IMessage
	{
	public:
		virtual RequestID GetRequestID() const = 0;
	};

	class IRequestIncomingMessage : public IRequestMessage
	{
	public:
		virtual void Act(IRequestInfo& reqInfo) = 0;
	};

	class IRequestOutgoingMessage : public IRequestMessage
	{
	public:
		virtual void Act(IRequestHandler& reqHandler, messages::DispatchID clientID) = 0;
	};
}