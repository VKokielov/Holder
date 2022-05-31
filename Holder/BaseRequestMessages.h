#pragma once

#include "IProxy.h"
#include "IRequestResponse.h"
#include "MessageTypeTags.h"

namespace holder::reqresp
{

	class RequestMessage : public IRequestMessage
	{
	public:
		RequestID GetRequestID() const override;

	protected:
		RequestMessage(RequestID requestID)
			:m_requestID(requestID)
		{ }

	private:
		RequestID m_requestID;
	};

	// Messages going TO request handler
	class RequestOutgoingMessage : public IRequestOutgoingMessage,
		public RequestMessage
	{
	public:
		base::types::TypeTag GetTag() const;
	protected:
		RequestOutgoingMessage(RequestID requestID)
			:RequestMessage(requestID)
		{ }
	};

	class RequestIncomingMessage : public IRequestIncomingMessage,
		public RequestMessage
	{
	public:
		base::types::TypeTag GetTag() const;
	protected:
		RequestIncomingMessage(RequestID requestID)
			:RequestMessage(requestID)
		{ }
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

		void Act(IRequestHandler& reqHandler, messages::DispatchID clientID) override
		{
			// Some kind of runtime dispatch needs to happen in this kind of multi-type
			// handling.  Doing it this way may go against the old C++ grain, but
			// it is far cleaner and more elegant than back-hack solutions which
			// merely render the dispatch explicit, without cutting it out

			auto& rHandler
				= dynamic_cast<ITypedRequestHandler<RequestInitializer>&>(reqHandler);
			rHandler.CreateRequest(m_requestInitializer, GetRequestID(), clientID);
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

		void Act(IRequestHandler& reqHandler, messages::DispatchID clientID) override
		{
			reqHandler.CancelRequest(GetRequestID(), clientID);
		}
	};

	// Messages arriving FROM request handler

	// Request state update.
	// See standard deltas
	template<typename RequestDelta>
	class RequestStateUpdate : public RequestIncomingMessage
	{
	public:
		RequestStateUpdate(RequestID requestID, 
			RequestDelta&& requestDelta)
			:RequestIncomingMessage(requestID),
			m_requestDelta(std::move(requestDelta))
		{ }

		void Act(IRequestInfo& reqInfo) override
		{
			m_requestDelta(reqInfo);
		}

	private:
		RequestDelta m_requestDelta;
	};

}