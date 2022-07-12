#include "BaseServiceCommService.h"

namespace impl_ns = holder::scomm;

bool impl_ns::BaseServiceCommService::GetInternalSubID(holder::messages::DispatchID client,
	SubscriptionID subID,
	InternalSubscriptionID& outID) const
{
	// Look up the subscription
	auto itClient = m_clientMap.find(client);

	if (itClient == m_clientMap.end())
	{
		return false;
	}

	return itClient->second.FindSubscription(subID, outID);
}

bool impl_ns::BaseServiceCommService::GetInternalReqID(messages::DispatchID client,
	RequestID reqID,
	InternalRequestID& outID) const
{
	// Look up the request
	auto itClient = m_clientMap.find(client);

	if (itClient == m_clientMap.end())
	{
		return false;
	}

	return itClient->second.FindRequest(reqID, outID);
}

