#include "BaseRequestInfo.h"

namespace impl_ns = holder::reqresp;

void impl_ns::BaseRequestInfo::SetState(impl_ns::RequestState newState)
{
	if (newState == m_requestState)
	{
		return;
	}

	// Cancels for requests in Completed state are ignored
	if (!CanBeCanceled() && IsCancelState(newState))
	{
		return;
	}

	if (m_pListener)
	{
		m_pListener->OnRequestStateChange(m_requestID, m_responseID,
			m_requestState, newState);
	}

	m_requestState = newState;
}

impl_ns::BaseRequestInfo::~BaseRequestInfo()
{
	if (m_pListener)
	{
		m_pListener->OnRequestPurged(m_requestID, m_responseID);
	}
}

void impl_ns::BaseRequestInfo::SetResult(std::shared_ptr<holder::base::IAppObject> pResult)
{
	m_pResult = pResult;
}