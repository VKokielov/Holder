#pragma once

#include "IAppObject.h"

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
		Timeout
	};

	class IRequestIssuer : public base::IAppObject
	{
	public:
		virtual RequestState GetRequestState(RequestID requestID) const = 0;
		virtual const std::shared_ptr<base::IAppObject>
			GetRequestResult(RequestID requestID) const = 0;
		virtual bool CancelRequest(RequestID requestID) = 0;
	};

	class IRequestListener : public base::IAppObject
	{
	public:
		virtual void OnRequestStateChange(RequestID reqId,
			ResponseID respId,
			RequestState oldState,
			RequestState newState) = 0;
	};

	struct RequestDescription
	{
		std::shared_ptr<IRequestListener> pListener;
		ResponseID respId;
		unsigned long usTimeout{ 0 };  // 0 means no timeout
	};

}