#pragma once

#include "IServiceComm.h"
#include "Messaging.h"
#include "MessageLib.h"
#include "ServiceCommMessages.h"
#include "MessageTypeTags.h"

#include <unordered_map>

namespace holder::scomm
{
	using InternalSubscriptionID = uint64_t;
	using InternalRequestID = uint64_t;

	enum class SubscriptionError
	{
		MissingSubID,
		DuplicateSubID,
	};

	enum class RequestError
	{
		Default
	};

	class BaseServiceCommService : public SCommService
	{
	private:
		class Client
		{
		public:
			std::weak_ptr<messages::ISenderEndpoint> m_pRemoteEndpoint;

			// Subscriptions
			std::unordered_map<SubscriptionID, InternalSubscriptionID>
				m_subMap;
			// Requests
			std::unordered_map<RequestID, InternalRequestID>
				m_reqMap;
		};

		class Subscription
		{

		private:
			SubscriptionID m_subscriptionID;
			messages::DispatchID m_clientID;
			std::weak_ptr<messages::ISenderEndpoint> m_pRemoteEndpoint;
		};

		class Request
		{

		private:
			RequestID m_requestID;
			messages::DispatchID m_clientID;
			std::weak_ptr<messages::ISenderEndpoint> m_pRemoteEndpoint;

			bool m_completed{ false };
		};

	public:
		void CancelRequest(messages::DispatchID clientID,
			RequestID requestID,
			unsigned long cancelReason) override;
		void Unsubscribe(messages::DispatchID clientID,
			SubscriptionID subID) override;

	protected:

		bool GetInternalSubID(messages::DispatchID client,
			SubscriptionID subID,
			InternalSubscriptionID& outID) const;

		bool GetInternalReqID(messages::DispatchID client,
			RequestID reqID,
			InternalRequestID& outID) const;

		InternalRequestID CreateRequestState(messages::DispatchID clientID,
			RequestID requestID);

		InternalSubscriptionID CreateSubscriptionState(messages::DispatchID clientID,
			SubscriptionID subscriptionID);

		void AddClient(messages::DispatchID clientID,
			std::shared_ptr<messages::ISenderEndpoint> pRemoteEndpoint);
		void RemoveClient(messages::DispatchID clientID);

		template<typename EventMessage, typename ... Args>
		void SendSubscriptionEvent(InternalSubscriptionID subID,
			Args&& ... args)
		{

		}

		void CompleteRequest(InternalRequestID reqID,
			bool success,
			std::shared_ptr<base::IAppObject> pResult);

		virtual void OnSubscriptionError(messages::DispatchID clientID,
			SubscriptionID subID,
			SubscriptionError subError);

		virtual void OnRequestError(messages::DispatchID clientID,
			RequestID requestID,
			RequestError reqError);

		// Messaging
		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			TagDispatch& tagDispatchTable)
		{
			tagDispatchTable.AddDispatch(base::constants::GetSCommClientMessageTag(),
				&BaseServiceCommService::OnSCommClientMessage);
		}

	private:
		bool m_diagnosticMode;

		std::unordered_map<messages::DispatchID,
			Client>  m_clientMap;

		std::unordered_map<InternalSubscriptionID,
			Subscription> m_internalSubMap;
		std::unordered_map<InternalRequestID,
			Request> m_internalReqMap;
	};
}