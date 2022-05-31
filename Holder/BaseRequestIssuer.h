#pragma once

#include "IRequestResponse.h"
#include "BaseRequestMessages.h"
#include "MessageLib.h"
#include "BaseRequestInfo.h"
#include "ExecutionManager.h"
#include "MessageTypeTags.h"
#include "Messaging.h"
#include "RequestDeltas.h"
#include <unordered_map>

namespace holder::reqresp
{
	class RequestStateException { };

	template<typename RequestInfo>
	class BaseRequestIssuer : public IRequestIssuer
	{
	private:
		static_assert(std::is_base_of_v<BaseRequestInfo, RequestInfo>, "RequestInfo must derive from BaseRequestInfo");	

		class TimeoutSender
			: public base::ITimerCallback
		{
		public:
			TimeoutSender(std::shared_ptr<messages::ISenderEndpoint> pEndpoint,
				RequestID requestID)
				:m_pEndpoint(pEndpoint),
				m_requestID(requestID)
			{ }

			void OnTimer(base::TimerUserID timerUserId, base::TimerID timerID) override
			{
				// The standard delta function object setting the request state to "canceled timeout"
				using DeltaSetCancel = RequestSetStateDelta<RequestState::CanceledTimeout>;

				// Send a message to the request issuer signifying that the request timed out
				// NOTE:  If the request is in completed or failed state after the timeout, the 
				// cancel timeout setter is ignored.
				messages::SendMessage < RequestStateUpdate<BaseRequestInfo,
					DeltaSetCancel> > (m_pEndpoint, DeltaSetCancel(), m_requestID);
			}

		private:
			std::shared_ptr<messages::ISenderEndpoint> m_pEndpoint;
			RequestID m_requestID;
		};
	public:

		const IRequestInfo* GetRequestInfo(RequestID requestID) const override
		{
			return GetRequestFromID(requestID);
		}

		bool CancelRequest(RequestID requestID) override
		{
			auto pRequest = GetRequestFromID(requestID);
			if (pRequest || !pRequest->CanBeCanceled() )
			{
				return false;
			}
			// Send a cancel message to the remote endpoint
			if (!m_pRemoteEndpoint)
			{
				throw RequestStateException();
			}
			messages::SendMessage<RequestCancelMessage>(m_pRemoteEndpoint, requestID);
			
			pRequest->SetState(RequestState::CanceledUser);
			return true;
		}

		bool PurgeRequest(RequestID requestID) override
		{
			// Remove the request from the table.  Further messages about the request on the other
			// side will be ignored
			// In case the request is in Issued state, trigger a Cancel message to the other side
			// in order to avoid extra work

			auto itRequest = m_requestMap.find(requestID);
			if (itRequest == m_requestMap.end())
			{
				return false;
			}

			if (itRequest->second.CanBeCanceled())
			{
				if (!m_pRemoteEndpoint)
				{
					throw RequestStateException();
				}
				service::SendMessage<RequestCancelMessage>(m_pRemoteEndpoint, requestID);
			}

			m_requestMap.erase(itRequest);
			return true;
		}

		void OnIncomingRequestMessage(messages::IMessage& msg,
			messages::DispatchID dispID)
		{
			IRequestIncomingMessage& typedMsg = static_cast<IRequestIncomingMessage&>(msg);

			RequestID requestID = typedMsg.GetRequestID();
			auto pRequest = GetRequestFromID(requestID);

			if (!pRequest)
			{
				// Nothing I can do
				return;
			}

			auto prevRequestState = pRequest->GetRequestStateNP();
			typedMsg.Act(*pRequest);
			auto curRequestState = pRequest->GetRequestStateNP();

			if (prevRequestState != curRequestState)
			{
				if (curRequestState == RequestState::CanceledTimeout)
				{
					// Send a message to the remote indicating the request has been canceled due to timeout
					messages::SendMessage<RequestCancelMessage>(m_pRemoteEndpoint, requestID);
				}

				if (m_purgeOnCancelAck
					&& curRequestState == RequestState::CancelAcknowledged)
				{
					PurgeRequest(pRequest->GetRequestIDNP());
				}
			}
			
		}

	protected:
		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			TagDispatch& tagDispatchTable)
		{
			tagDispatchTable.AddDispatch(base::constants::GetRequestIncomingMessageTag(),
				&BaseRequestIssuer<RequestInfo>::OnIncomingRequestMessage);
		}

		void SetRemoteEndpoint(std::shared_ptr<messages::ISenderEndpoint> pEndpoint)
		{
			m_pRemoteEndpoint = pEndpoint;
		}

		void SetLocalEndpoint(const std::shared_ptr<messages::ISenderEndpoint>& pEndpoint)
		{
			m_pLocalEndpoint = pEndpoint;
		}

		template<typename RequestInitializer>
		RequestID IssueRequest(RequestInitializer&& reqData,
			const std::shared_ptr<messages::ISenderEndpoint>& pRequestHandler,
			unsigned long timeout)
		{
			// Create a request and send a message to the remote with the request
			// Also set a oneshot timer for timeouts, if one is specified
			if (!m_pRemoteEndpoint)
			{
				throw RequestStateException();
			}

			RequestID newRequestID = m_nextRequestID++;

			auto emplResult = m_requestMap.emplace(newRequestID,
				RequestInfo(reqData));

			// Send an outgoing request message to the other side
			messages::SendMessage<RequestIssueMessage<RequestInitializer> >
				(m_pRemoteEndpoint, std::move(reqData), newRequestID);

			if (timeout > 0)
			{
				auto pLocalEndpoint = m_pLocalEndpoint.lock();

				if (pLocalEndpoint)
				{
					// Set an anonymous timeout timer
					base::ExecutionManager::GetInstance().SetTimer("", timeout, false, 0,
						std::make_shared<TimeoutSender>(pLocalEndpoint, newRequestID));
				}
			}

			return newRequestID;
		}

	private:
		RequestInfo* GetRequestFromID(RequestID requestID)
		{
			auto itRequest = m_requestMap.find(requestID);

			if (itRequest == m_requestMap.end())
			{
				return nullptr;
			}

			return &itRequest->second;
		}

		// Automatically purge a request if it was cancel-acked
		bool m_purgeOnCancelAck{ false };

		std::shared_ptr<messages::ISenderEndpoint> m_pRemoteEndpoint;
		std::weak_ptr<messages::ISenderEndpoint> m_pLocalEndpoint;
		std::unordered_map<RequestID, RequestInfo> m_requestMap;
		RequestID m_nextRequestID{ 0 };

		unsigned long m_timeoutID{ 0 };
	};


}