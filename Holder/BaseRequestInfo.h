#pragma once

#include "IRequestResponse.h"

#include <memory>

namespace holder::reqresp
{
	class BaseRequestInfo : public IRequestInfo
	{
	public:
		class Updater
		{
		public:
			void SetNewState(RequestState reqState)
			{
				m_isValid.reset();
				m_newState.emplace(reqState);
			}
			void ClearNewState()
			{
				m_isValid.reset();
				m_newState.reset();
			}
			void SetResult(std::shared_ptr<base::IAppObject> pResult)
			{
				m_isValid.reset();
				m_reqResult.emplace(std::move(pResult));
			}

			bool IsValid() const;

			// Apply
			void operator ()();
		private:
			Updater(BaseRequestInfo& target)
				:m_target(target)
			{ }

			BaseRequestInfo& m_target;

			// The sequence number tells us whether the states have changed
			// between updates
			mutable std::optional<bool> m_isValid;
			mutable unsigned long m_seqValidated;

			std::optional<RequestState> m_newState;
			std::optional<std::shared_ptr<base::IAppObject> > m_reqResult;

			friend class BaseRequestInfo;			
		};

		Updater GetUpdater()
		{
			return Updater(*this);
		}

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
			return requestState == RequestState::Issued
				|| IsCancelState(requestState);
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

		// For updaters
		unsigned int m_sequence{ 0 };
	};
}