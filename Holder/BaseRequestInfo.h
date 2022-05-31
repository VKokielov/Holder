#pragma once

#include "IRequestResponse.h"

#include <memory>

namespace holder::reqresp
{
	class BaseRequestInfo : public IRequestInfo
	{
	public:
		void SetState(RequestState newState);
		void SetResult(std::shared_ptr<base::IAppObject> pResult);

		// Inform the listener, if there is one, that the request is being purged
		~BaseRequestInfo();

		// Non-polymorphic version of GetRequestState
		RequestState GetRequestStateNP() const { return m_requestState; }
		RequestID GetRequestIDNP() const { return m_requestID; }

		static bool IsCancelState(RequestState requestState)
		{
			return requestState == RequestState::CancelAcknowledged
				|| requestState == RequestState::CanceledTimeout
				|| requestState == RequestState::CanceledUser;
		}

		static bool IsCancelableState(RequestState requestState)
		{
			return requestState == RequestState::Issued;
		}

		bool CanBeCanceled() const
		{
			return IsCancelableState(m_requestState);
		}

	protected:
		BaseRequestInfo(const std::shared_ptr<IRequestListener> pListener,
			RequestID requestID,
			ResponseID responseID)
			:m_pListener(pListener),
			m_requestID(requestID),
			m_responseID(responseID)
		{

		}

	private:
		std::shared_ptr<IRequestListener> m_pListener;
		std::shared_ptr<base::IAppObject> m_pResult;
		RequestID m_requestID;
		ResponseID m_responseID;

		RequestState m_requestState{ RequestState::Issued };
	};
}