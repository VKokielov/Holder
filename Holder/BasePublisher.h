#pragma once

#include "IPublishSubscribe.h"
#include <unordered_set>
#include <functional>

namespace holder::pubsub
{
	class BasePublisher
	{
	private:
		struct Subscription
		{
			std::shared_ptr<base::IAppObject> pListener;
			UserSubscriptionID userSubID;
		};

	protected:
		class SubscriptionSet
		{
		public:

			size_t Size() const { return m_subs.size(); }

			template<typename Listener, typename ... Args>
			void operator()(Args&&...args)
			{
				for ()
			}

			void AddSub(ServiceSubscriptionID subID)
			{

			}

			void RemoveSub(ServiceSubscriptionID subID)
			{

			}
		private:
			std::unordered_set<ServiceSubscriptionID> m_subs;
		};


	private:
		Subscription* GetSubscription(ServiceSubscriptionID subID);
	};
 


}