#pragma once

#include "IProxy.h"
#include "IRequestResponse.h"

namespace holder::reqresp
{


	// Messages going TO request handler
	class RequestOutgoingMessage : public service::IServiceMessage
	{
	protected:
		RequestOutgoingMessage(RequestID requestID)
			:m_requestID(requestID)
		{ }

		RequestID GetRequestID() const { return m_requestID; }
	private:
		RequestID m_requestID;
	};

	// Issue request
	template<typename RequestInitializer>
	class RequestIssueMessage : public RequestOutgoingMessage
	{
	public:
		RequestIssueMessage(RequestInitializer&& requestInitializer,
			RequestID requestID)
			:RequestOutgoingMessage(requestID),
			m_requestInitializer(std::move(requestInitializer))
		{ }

		void Act(service::IServiceLink& object) override
		{
			// Some kind of runtime dispatch needs to happen in this kind of multi-type
			// handling.  Doing it this way may go against the old C++ grain, but
			// it is far cleaner and more elegant than back-hack solutions which
			// merely the dispatch explicit, without cutting it out
			auto& rHandler
				= dynamic_cast<ITypedRequestHandler<RequestInitializer>&>(object);
			rHandler.CreateRequest(m_requestInitializer, GetRequestID());
		}
	private:
		RequestInitializer m_requestInitializer;
	};

	// Cancel request
	class RequestCancelMessage : public RequestOutgoingMessage
	{
	public:
		RequestCancelMessage(RequestID requestID)
			:RequestOutgoingMessage(requestID)
		{ }

		void Act(service::IServiceLink& object) override
		{
			auto& rHandler
				= dynamic_cast<IRequestHandler&>(object);
			rHandler.CancelRequest(GetRequestID());
		}
	};

	// Messages arriving FROM request handler

	// Request state update.
	// See standard deltas
	template<typename RequestInfo, typename RequestDelta>
	class RequestStateUpdate : public IRequestMessage
	{
	public:
		RequestID GetRequestID() const override { return m_requestID; }

		void Act(IRequestInfo& reqInfo) override
		{
			m_requestDelta(static_cast<RequestInfo&>(reqInfo));
		}

	private:
		RequestID m_requestID;
		RequestDelta m_requestDelta;
	};

}