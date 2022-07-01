#pragma once

#include "IServiceComm.h"
#include "Messaging.h"
#include "MessageLib.h"
#include "ServiceCommMessages.h"
#include "ExecutionManager.h"
#include "MessageTypeTags.h"
#include <memory>

namespace holder::scomm
{
	using ServiceCommInternalID = size_t;

	class BaseServiceCommClient : public SCommClient
	{
	private:
		struct Subscription_
		{
			std::shared_ptr<ISubscriber>  pSubscriber;
			UserSubscriptionID userSubID;
		};

		class TimeoutSender
			: public base::ITimerCallback
		{
		public:
			TimeoutSender(std::shared_ptr<messages::ISenderEndpoint> pServiceEndpoint,
				RequestID requestID)
				:m_pServiceEndpoint(pServiceEndpoint),
				m_requestID(requestID)
			{ }

			void OnTimer(base::TimerUserID timerUserId, base::TimerID timerID) override
			{
				// Send a cancel request message to the service
				messages::SendMessage <CancelRequestMessage>(m_pServiceEndpoint, m_requestID, 
					CANCEL_TIMEOUT);
			}

		private:
			std::shared_ptr<messages::ISenderEndpoint> m_pServiceEndpoint;
			RequestID m_requestID;
		};

	public:
		void OnUnsubscribed(SubscriptionID subID) override;
		void DispatchToSubs(const SubscriptionEventMessage& evtMessage) override;

	protected:

		template<typename Service, typename SubscriptionInfo>
		bool Subscribe(SubscriptionInfo&& subInfo,
			SubscriptionParams<typename SubscriptionInfo::TSubscriber>&& subParams,
			SubscriptionID& outID)
		{
			auto pRemoteEndpoint = m_pRemoteEndpoint.lock();
			if (!pRemoteEndpoint)
			{
				return false;
			}

			SubscriptionID subID = CreateSubscription(std::move(subParams.pSubscriber),
				subParams.userSubID);

			messages::SendMessage<SubscribeMessage<Service, SubscriptionInfo> >
				(pRemoteEndpoint, subID, std::move(subInfo));

			return true;
		}

		template<typename Service, typename RequestInfo>
		bool SendRequest(RequestInfo&& reqInfo,
			SubscriptionParams<IRequestSubscriber>&& reqSubParams,
			unsigned long timeout,
			RequestIDs& outIDs)
		{
			auto pRemoteEndpoint = m_pRemoteEndpoint.lock();
			if (!pRemoteEndpoint)
			{
				return false;
			}

			SubscriptionID subID{ INVALID_SUB };
			if (reqSubParams.pSubscriber)
			{
				subID = CreateSubscription(std::move(reqSubParams.pSubscriber),
					reqSubParams.userSubID);
			}

			RequestID reqID = ++m_nextRequestID;
			messages::SendMessage<RequestMessage<Service, RequestInfo> >
				(pRemoteEndpoint, reqID, subID, std::move(reqInfo));

			if (timeout != 0)
			{
				// Set an anonymous one-shot timer with a cancel message on timeout
				base::ExecutionManager::GetInstance().SetTimer("", timeout, false, 0,
					std::make_shared<TimeoutSender>(pRemoteEndpoint, reqID));
			}

			outIDs.requestID = reqID;
			outIDs.requestSubID = subID;

			return true;
		}

		// Send a message to cancel a request.  Since we do not maintain request state in the 
		// base class, it's the caller's responsibility to ensure that the request ID is valid.
		bool DoCancelRequest(RequestID reqID);

		bool DoUnsubscribe(SubscriptionID subID);

		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			TagDispatch& tagDispatchTable)
		{
			tagDispatchTable.AddDispatch(base::constants::GetSCommServiceMessageTag(),
				&BaseServiceCommClient::OnSCommServiceMessage);
		}

		void SetRemoteEndpoint(std::shared_ptr<messages::ISenderEndpoint> pEndpoint)
		{
			m_pRemoteEndpoint = pEndpoint;
		}


	private:
		SubscriptionID CreateSubscription(std::shared_ptr<ISubscriber> pSubscriber,
			UserSubscriptionID subID);

		void OnSCommServiceMessage(messages::IMessage& msg,
			messages::DispatchID dispID);

		std::unordered_map<SubscriptionID, Subscription_> m_subMap;

		SubscriptionID m_nextSubscriptionID{ 0 };
		RequestID m_nextRequestID{ 0 };
		std::weak_ptr<messages::ISenderEndpoint> m_pRemoteEndpoint;
	};

}