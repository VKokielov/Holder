#pragma once

#include "IAppObject.h"

#include <cinttypes>
#include <memory>

namespace holder::pubsub
{
	using ServiceSubscriptionID = uint64_t;
	using UserSubscriptionID = uint64_t;

	struct SubscriptionDetails 
	{ 
		UserSubscriptionID connID;
	};

	class ISubscriber : public virtual base::IAppObject
	{
	public:
		
	};

	class IPublisher : public virtual base::IAppObject
	{
	public:
		virtual ServiceSubscriptionID Subscribe(const std::shared_ptr<ISubscriber>& pListener,
			const SubscriptionDetails& subDetails) = 0;
		virtual void Unsubscribe(ServiceSubscriptionID subID) = 0;
	};
}