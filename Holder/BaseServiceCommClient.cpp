#include "BaseServiceCommClient.h"

namespace impl_ns = holder::scomm;

impl_ns::SubscriptionID impl_ns::BaseServiceCommClient::CreateSubscription(std::shared_ptr<ISubscriber> pSubscriber,
	impl_ns::UserSubscriptionID subID)
{
	SubscriptionID subID = m_nextSubscriptionID++;

	auto emplResult = m_subMap.emplace(subID, Subscription_());

	emplResult.first->second.pSubscriber = pSubscriber;
	emplResult.first->second.userSubID = subID;
}

void impl_ns::BaseServiceCommClient::OnUnsubscribed(SubscriptionID subID)
{
	// Find the subscription
	auto itSub = m_subMap.find(subID);

	if (itSub != m_subMap.end())
	{
		// Notify the subscription that it is being removed
		itSub->second.pSubscriber->OnUnsubscribe(itSub->second.userSubID);
	}

	// Remove the subscription
	m_subMap.erase(itSub);
}
void impl_ns::BaseServiceCommClient::DispatchToSubs(const SubscriptionEventMessage& evtMessage)
{
	auto itSub = m_subMap.find(evtMessage.GetSubID());

	// If the subscription is not in the map, that means it was already cancelled
	if (itSub != m_subMap.end())
	{
		evtMessage.DispatchToSubscription(*itSub->second.pSubscriber.get(),
			itSub->second.userSubID);
	}
}

bool impl_ns::BaseServiceCommClient::DoCancelRequest(RequestID reqID)
{
	auto pRemoteEndpoint = m_pRemoteEndpoint.lock();
	if (!pRemoteEndpoint)
	{
		return false;
	}

	messages::SendMessage <CancelRequestMessage>(pRemoteEndpoint, reqID,
		CANCEL_CLIENT_REQUEST);

	return true;
}

bool impl_ns::BaseServiceCommClient::DoUnsubscribe(SubscriptionID subID)
{
	auto itSub = m_subMap.find(subID);
	if (itSub == m_subMap.end())
	{
		return false;
	}

	auto pRemoteEndpoint = m_pRemoteEndpoint.lock();
	if (!pRemoteEndpoint)
	{
		return false;
	}

	itSub->second.pSubscriber->OnUnsubscribe(itSub->second.userSubID);

	// Send a message and remove the subscription
	messages::SendMessage<UnsubscribeMessage>(pRemoteEndpoint, subID);

	m_subMap.erase(itSub);
}

void impl_ns::BaseServiceCommClient::OnSCommServiceMessage(messages::IMessage& msg,
	messages::DispatchID dispID)
{
	SCommServiceMessage& servMessage
		= static_cast<SCommServiceMessage&>(msg);

	servMessage.Act(*this);
}