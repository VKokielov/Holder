#pragma once

#include "IServiceComm.h"
#include "Messaging.h"

namespace holder::scomm
{
	class SubscriptionEventMessage;

	enum class RequestChange
	{
		Canceled,
		Completed
	};

	class SCommClient
	{
	public:
		virtual void OnUnsubscribed(SubscriptionID subID) = 0;
		virtual void DispatchToSubs(const SubscriptionEventMessage& evtMessage) = 0;
	};

	class SCommService
	{
	public:

		virtual void CancelRequest(messages::DispatchID clientID,
			RequestID requestID,
			unsigned long cancelReason) = 0;
		virtual void Unsubscribe(messages::DispatchID clientID,
			SubscriptionID subID) = 0;
	};

	class SCommServiceMessage : public messages::IMessage
	{
	public:
		base::types::TypeTag GetTag() const override;

		virtual void Act(SCommClient& client) = 0;
	};

	class SCommClientMessage : public messages::IMessage
	{
	public:
		base::types::TypeTag GetTag() const override;

		virtual void Act(SCommService& service, 
			messages::DispatchID clientID) = 0;
	};

	// Specific message types
	
	// Server to client
	class SubscriptionEventMessage : public SCommServiceMessage
	{
	public:
		SubscriptionID GetSubID() const;
		void Act(SCommClient& client) override;
		virtual void DispatchToSubscription(ISubscriber& sub,
			UserSubscriptionID subID) const = 0;

	protected:
		SubscriptionEventMessage(SubscriptionID subID, bool unsubscribe)
			:m_subID(subID),
			m_unsubscribe(unsubscribe)
		{ }
	private:
		SubscriptionID m_subID;
		bool m_unsubscribe;
	};

	class RequestEventMessage : public SubscriptionEventMessage
	{
	public:
		RequestEventMessage(SubscriptionID subID,
			RequestChange reqChange,
			RequestID reqID,
			bool success,
			std::shared_ptr<base::IAppObject> pResult,
			unsigned long cancelReason,
			bool unsubscribe)
			:SubscriptionEventMessage(subID, unsubscribe),
			m_change(reqChange),
			m_requestID(reqID),
			m_success(success),
			m_pResult(pResult),
			m_cancelReason(cancelReason)
		{ }

		void DispatchToSubscription(ISubscriber& sub,
			UserSubscriptionID subID) const override;

	private:
		RequestChange m_change;
		RequestID m_requestID;
		bool m_success;
		std::shared_ptr<base::IAppObject> m_pResult;
		unsigned long m_cancelReason;  // if relevant
	};

	// Client to server
	template<typename Service, typename SubscriptionInfo>
	class SubscribeMessage : public SCommClientMessage
	{
	public:
		SubscribeMessage(SubscriptionID subID, SubscriptionInfo&& subInfo)
			:m_subID(subID),
			m_subInfo(subInfo)
		{ }

		static_assert(std::is_base_of_v<SCommService, Service>,
			"Service must derive from SCommService");

		void Act(SCommService& service, 
			messages::DispatchID clientID) override
		{
			Service& typedService = static_cast<Service&>(service);

			typedService.CreateSubscription(clientID, std::move(m_subInfo));
		}

	private:
		SubscriptionID m_subID;
		SubscriptionInfo m_subInfo;
	};

	class UnsubscribeMessage : public SCommClientMessage
	{
	public:
		UnsubscribeMessage(SubscriptionID subID)
			:m_subID(subID)
		{ }

		void Act(SCommService& service,
			messages::DispatchID clientID) override;
	private:
		SubscriptionID m_subID;
	};

	template<typename Service, typename RequestInfo>
	class RequestMessage : public SCommClientMessage
	{
	public:
		RequestMessage(RequestID reqID,
			SubscriptionID subID,
			RequestInfo&& reqInfo)
			:m_requestID(reqID),
			m_subscriptionID(subID),
			m_reqInfo(std::move(reqInfo))
		{ }

		static_assert(std::is_base_of_v<SCommService, Service>,
			"Service must derive from SCommService");

		void Act(SCommService& service,
			messages::DispatchID clientID) override
		{
			Service& typedService = static_cast<Service&>(service);

			typedService.CreateRequest(clientID,
				m_requestID,
				std::move(m_reqInfo),
				m_subscriptionID);
		}
	private:
		RequestID m_requestID;
		SubscriptionID m_subscriptionID;
		RequestInfo m_reqInfo;
	};

	class CancelRequestMessage : public SCommClientMessage
	{
	public:
		CancelRequestMessage(RequestID reqID)
			:m_requestID(reqID)
		{ }

		void Act(SCommService& service,
			messages::DispatchID clientID) override;

	private:
		RequestID m_requestID;
		unsigned int m_cancelReason;
	};

}