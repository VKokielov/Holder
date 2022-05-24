#pragma once

#include "IRequestResponse.h"
#include "BaseRequestMessages.h"
#include "ServiceMessageLib.h"
#include "BaseRequestInfo.h"
#include "ExecutionManager.h"
#include <unordered_map>

namespace holder::reqresp
{
	class RequestStateException { };

	template<typename RequestInfo>
	class BaseRequestIssuer : public IRequestIssuer
	{
	private:
		static_assert(std::is_base_of_v<BaseRequestInfo, RequestInfo>, "RequestInfo must derive from BaseRequestInfo");	
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
			service::SendMessage<RequestCancelMessage>(m_pRemoteEndpoint, requestID);
			
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

	protected:
		void SetRemoteEndpoint(std::shared_ptr<messages::ISenderEndpoint> pEndpoint)
		{
			m_pRemoteEndpoint = pEndpoint;
		}

		template<typename RequestInitializer>
		RequestID IssueRequest(RequestInfo&& reqData,
			const std::shared_ptr<messages::ISenderEndpoint>& pRequest,
			unsigned long timeout)
		{
			// Create a request and send a message to the remote with the request
			// Also set a oneshot timer for timeouts, if one is specified
			if (!m_pRemoteEndpoint)
			{
				throw RequestStateException();
			}

			RequestID newRequestID = m_nextRequestID;


		}

		void OnRequestMessage(const IRequestMessage& requestMessage)
		{
			auto pRequest = GetRequestFromID(requestMessage.GetRequestID());

			if (!pRequest)
			{
				// Nothing I can do
				return;
			}

			requestMessage.Act(*pRequest);

			if (m_purgeOnCancelAck 
				&& pRequest->GetRequestStateNP() == RequestState::CancelAcknowledged)
			{
				PurgeRequest(pRequest->GetRequestIDNP());
			}
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
		std::unordered_map<RequestID, RequestInfo> m_requestMap;
		RequestID m_nextRequestID{ 0 };
	};


}