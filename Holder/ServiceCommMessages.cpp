#include "ServiceCommMessages.h"
#include "MessageTypeTags.h"

namespace impl_ns = holder::scomm;

holder::base::types::TypeTag impl_ns::SCommServiceMessage::GetTag() const
{
	return base::constants::GetSCommServiceMessageTag();
}

holder::base::types::TypeTag impl_ns::SCommClientMessage::GetTag() const
{
	return base::constants::GetSCommClientMessageTag();
}

impl_ns::SubscriptionID impl_ns::SubscriptionEventMessage::GetSubID() const
{
	return m_subID;
}

void impl_ns::SubscriptionEventMessage::Act(SCommClient& client)
{
	client.DispatchToSubs(*this);

	if (m_unsubscribe)
	{
		client.OnUnsubscribed(m_subID);
	}
}

void impl_ns::RequestEventMessage::DispatchToSubscription(ISubscriber& sub,
	UserSubscriptionID subID) const 
{
	IRequestSubscriber& reqSub = static_cast<IRequestSubscriber&> (sub);

	if (m_change == RequestChange::Canceled)
	{
		reqSub.OnRequestCanceled(subID, m_requestID, m_cancelReason);
	}
	else if (m_change == RequestChange::Completed)
	{
		if (m_success)
		{
			reqSub.OnRequestCompleted(subID, m_requestID, m_pResult);
		}
		else
		{
			reqSub.OnRequestFailed(subID, m_requestID);
		}
	}
}

void impl_ns::UnsubscribeMessage::Act(SCommService& service,
	messages::DispatchID clientID)
{
	service.Unsubscribe(clientID, m_subID);
}

void impl_ns::CancelRequestMessage::Act(SCommService& service,
	messages::DispatchID clientID)
{
	service.CancelRequest(clientID, m_requestID, m_cancelReason);
}
