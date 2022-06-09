#include "BaseRequestInfo.h"

namespace impl_ns = holder::reqresp;

bool impl_ns::BaseRequestInfo::Updater::IsValid() const
{
	if (m_isValid.has_value() 
		&& m_seqValidated == m_target.m_sequence)
	{
		return *m_isValid;
	}

	m_seqValidated = m_target.m_sequence;

	RequestState stateToBe = m_newState.has_value() ? m_newState.value()
		: m_target.m_requestState;

	bool hasResult = m_reqResult.has_value() ? (bool)m_reqResult.value()
		: (bool)m_target.m_pResult;

	if (stateToBe != RequestState::Completed
		&& stateToBe != RequestState::Failed
		&& hasResult)
	{
		m_isValid.emplace(false);
		return false;
	}

	if (IsCancelState(stateToBe) && !IsCancelableState(m_target.m_requestState))
	{
		m_isValid.emplace(false);
		return false;
	}

	m_isValid.emplace(true);
	return true;
}

void impl_ns::BaseRequestInfo::Updater::operator ()()
{
	if (!IsValid())
	{
		return;
	}

	// Is there an actual change?
	bool hadChange{ false };

	RequestState oldState;
	if (m_newState.has_value())
	{
		hadChange = m_newState.value() != m_target.m_requestState;
		oldState = m_target.m_requestState;
		m_target.m_requestState = m_newState.value();
	}

	if (m_reqResult.has_value())
	{
		hadChange = hadChange || m_reqResult.value().get() != m_target.m_pResult.get();
		m_target.m_pResult = m_reqResult.value();
	}

	if (hadChange)
	{
		++m_target.m_sequence;

		if (m_target.m_pListener)
		{
			m_target.m_pListener->OnRequestStateChange(m_target.m_requestID, 
				m_target.m_responseID,
				oldState, m_target.m_requestState);
		}
	}
}

impl_ns::BaseRequestInfo::~BaseRequestInfo()
{
	if (m_pListener)
	{
		m_pListener->OnRequestPurged(m_requestID, m_responseID);
	}
}