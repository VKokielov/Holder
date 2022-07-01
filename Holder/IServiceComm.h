#pragma once

#include "IProxy.h"
#include <memory>
#include <type_traits>
#include <limits>

namespace holder::scomm
{
	class ISubscriber;

	using RequestID = uint32_t;
	using SubscriptionID = uint32_t;
	using UserSubscriptionID = uint32_t;

	constexpr SubscriptionID INVALID_SUB = std::numeric_limits<SubscriptionID>::max();
	
	constexpr unsigned long CANCEL_REASON_UNKNOWN = 0;
	constexpr unsigned long CANCEL_CLIENT_REQUEST = 1;
	constexpr unsigned long CANCEL_TIMEOUT = 999;

	struct RequestIDs
	{
		RequestID requestID;
		SubscriptionID requestSubID;
	};

	class ISubscriber : public virtual base::IAppObject
	{
	public:
		virtual void OnUnsubscribe(UserSubscriptionID userSubID) = 0;
	};

	template<typename SubscriberType>
	struct SubscriptionParams
	{
		static_assert(std::is_base_of_v<ISubscriber, SubscriberType>,
			"SubscriptionInfo: template parameter must derive from ISubscriber");

		std::shared_ptr<SubscriberType> pSubscriber;
		UserSubscriptionID userSubID;
	};


	class IRequestSubscriber : public ISubscriber
	{
	public:
		virtual void OnRequestCanceled(UserSubscriptionID userSubID,
			RequestID reqId,
			unsigned long cancelReason) = 0;
		virtual void OnRequestFailed(UserSubscriptionID userSubID,
			RequestID reqId) = 0;
		virtual void OnRequestCompleted(UserSubscriptionID userSubID,
			RequestID reqId,
			const std::shared_ptr<base::IAppObject>& pResult) = 0;
	};

	class IServiceCommProxy : public base::IAppObject
	{
	public:
		virtual bool CancelRequest(RequestID requestID) = 0;
		virtual bool Unsubscribe(SubscriptionID subscriptionID) = 0;
	};

}