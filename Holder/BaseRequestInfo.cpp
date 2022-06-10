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

	bool isValid{ false };
	if (stateToBe == RequestState::Issued)
	{
		isValid = false;
	}
	else if (stateToBe == RequestState::Completed || stateToBe == RequestState::Failed)
	{
		isValid = hasResult && m_target.m_requestState == RequestState::Issued);
	}
	else if (stateToBe == RequestState::CanceledTimeout || stateToBe == RequestState::CanceledUser)
	{
		isValid = m_target.m_requestState == RequestState::Issued;
	}
	else if (stateToBe == RequestState::CancelAcknowledged)
	{
		isValid = m_target.m_requestState == RequestState::CanceledTimeout
			|| m_target.m_requestState == RequestState::CanceledUser;
	}

	m_isValid.emplace(isValid);
	return isValid;
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

		auto pTargetListener = m_target.m_pListener.lock();

		if (pTargetListener)
		{
			if (m_newState.has_value())
			{
				auto newState = m_newState.value();
				if (newState == RequestState::Completed)
				{
					pTargetListener->OnRequestCompleted(m_target.m_requestID,
						m_target.m_responseID, m_target.m_pResult);
				}
				else if (newState == RequestState::Failed)
				{
					pTargetListener->OnRequestFailed(m_target.m_requestID, m_target.m_responseID);
				}
				else if (newState == RequestState::CancelAcknowledged)
				{
					pTargetListener->OnRequestCanceled(m_target.m_requestID,
						m_target.m_responseID);
				}
			}
		}
	}
}

impl_ns::BaseRequestInfo::~BaseRequestInfo()
{
	auto pListener = m_pListener.lock();
	if (pListener)
	{
		pListener->OnRequestPurged(m_requestID, m_responseID);
	}
}