#pragma once

#include "IRequestResponse.h"
#include "BaseRequestMessages.h"
#include "MessageLib.h"

#include <unordered_map>
#include <type_traits>

namespace holder::reqresp
{

	class BaseRequestHandler;

	using HandlerRequestInfoID = size_t;

	enum class HandlerRequestState
	{
		Active,
		Cancelled,
		Complete
	};

	struct BaseHandlerRIInitializer
	{
		std::weak_ptr<BaseRequestHandler> pOwner;
		HandlerRequestInfoID requestInfoID;
		messages::DispatchID clientID;
		RequestID requestID;
		std::weak_ptr<messages::ISenderEndpoint> pRemoteEndpoint;
	};

	class BaseHandlerRequestInfo
	{
	public:
		~BaseHandlerRequestInfo();

		// The public interface is to be used by the class deriving from the request handler
		void CompleteRequest(bool success,
							std::shared_ptr<base::IAppObject> pResult);
		
		template<typename RequestDelta>
		bool SendRequestUpdate(RequestDelta&& delta)
		{
			auto pRemoteEndpoint = m_pRemoteEndpoint.lock();

			if (!pRemoteEndpoint)
			{
				return false;
			}

			auto pRequestUpdate =
				std::make_shared<RequestStateUpdate<RequestDelta> >(m_requestID,
					std::move(delta));

			pRemoteEndpoint->SendMessage(std::move(pRequestUpdate));

			return true;
		}

		bool IsActive() const { return m_reqState == HandlerRequestState::Active; }

		HandlerRequestInfoID GetRequestInfoID() const { return m_requestInfoID; }
		messages::DispatchID GetClientID() const { return m_clientID; }
		RequestID GetRequestID() const { return m_requestID; }

	protected:
		
		BaseHandlerRequestInfo(const BaseHandlerRIInitializer& bhInitializer)
			:m_owner(bhInitializer.pOwner),
			m_requestInfoID(bhInitializer.requestInfoID),
			m_clientID(bhInitializer.clientID),
			m_requestID(bhInitializer.requestID),
			m_pRemoteEndpoint(bhInitializer.pRemoteEndpoint)
		{ }

		virtual void OnRequestCancelled();
		virtual void OnRequestComplete(bool success,
			const std::shared_ptr<base::IAppObject>& requestResult);
		// Request is being orphaned - the handler no longer owns it
		virtual void OnRequestOrphaned();

	private:
		void SetCancel();

		// Arguments
		std::weak_ptr<BaseRequestHandler> m_owner;

		// Coordinates
		HandlerRequestInfoID m_requestInfoID;
		messages::DispatchID m_clientID;
		RequestID m_requestID; 
		std::weak_ptr<messages::ISenderEndpoint> m_pRemoteEndpoint;

		HandlerRequestState m_reqState{ HandlerRequestState::Active };
		bool m_success{ false };
		std::shared_ptr<base::IAppObject>  m_pResult;

		friend class BaseRequestHandler;
	};

	class BaseRequestHandler : public IRequestHandler
	{
	private:
		enum class RequestIterCommand
		{
			Stop,
			Continue,
			Remove
		};

		class ClientInfo
		{
		public:
			ClientInfo(std::shared_ptr<messages::ISenderEndpoint> pRemoteEndpoint)
				:m_pRemoteEndpoint(std::move(pRemoteEndpoint))
			{ }

			bool AddRequest(RequestID requestID, HandlerRequestInfoID requestInfoID);
			bool RemoveRequest(RequestID requestID);

			bool HasRequestID(RequestID requestID) const;

			HandlerRequestInfoID GetRequestInfoID(RequestID requestID) const;

			const std::shared_ptr<messages::ISenderEndpoint>
				GetRemoteEndpoint()
			{
				return m_pRemoteEndpoint;
			}

			template<typename F>
			void IterateRequests(F&& callback)
			{
				for (auto itEntry = m_requestMap.begin();
					itEntry != m_requestMap.end();
					)
				{
					RequestIterCommand iterCommand = callback(itEntry->first, itEntry->second);

					if (iterCommand == RequestIterCommand::Stop)
					{
						break;
					}
					else if (iterCommand == RequestIterCommand::Remove)
					{
						itEntry = m_requestMap.erase(itEntry);
					}
					else
					{
						++itEntry;
					}
				}
			}
		private:
			std::unordered_map<RequestID,
				HandlerRequestInfoID> m_requestMap;
			// Other information
			std::shared_ptr<messages::ISenderEndpoint>
				m_pRemoteEndpoint;
		};

	public:
		void CancelRequest(RequestID requestID, messages::DispatchID clientID) override;
	protected:

		virtual std::shared_ptr<BaseRequestHandler>
			GetMyBaseRequestHandlerSharedPtr() = 0;

		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			TagDispatch& tagDispatchTable)
		{
			tagDispatchTable.AddDispatch(base::constants::GetRequestOutgoingMessageTag(),
				&BaseRequestHandler::OnRequestOutgoingMessage);
		}

		void AddClient(messages::DispatchID clientID,
			std::shared_ptr<messages::ISenderEndpoint> pRemoteEndpoint);
		void RemoveClient(messages::DispatchID clientID);

		template<typename HandlerRequestInfoType,
				 typename ... Args>
		HandlerRequestInfoID
			CreateRequest(messages::DispatchID clientID,
				RequestID requestID, 
				Args&& ... args)
		{
			static_assert(std::is_base_of_v<BaseHandlerRequestInfo, HandlerRequestInfoType>,
				"Your HandlerRequestInfo class must derive from BaseHandlerRequestInfo");
			HandlerRequestInfoID reqID{ 0 };

			// Make it harder to hack
			if (m_freeID == 0) 
			{
				return reqID;
			}

			// Find the client
			auto itClient = m_requestClients.find(clientID);
			if (itClient == m_requestClients.end())
			{
				return reqID;
			}

			HandlerRequestInfoID internalID = m_freeID++;

			// Client found.  Create the request
			BaseHandlerRIInitializer reqInitializer;
			reqInitializer.clientID = clientID;
			reqInitializer.requestID = requestID;
			reqInitializer.requestInfoID = internalID;
			reqInitializer.pOwner = GetMyBaseRequestHandlerSharedPtr();
			reqInitializer.pRemoteEndpoint = itClient->second.GetRemoteEndpoint();

			auto pRequest = std::make_shared<HandlerRequestInfoType>(reqInitializer,
				std::forward<Args>(args)...);

			itClient->second.AddRequest(requestID, internalID);
			m_requestInfos.emplace(internalID, pRequest);

			return internalID;
		}

		template<typename F>
		void IterateAllRequests(F&& callback)
		{
			for (auto& requestPair : m_requestInfos)
			{
				callback(requestPair.first, requestPair.second.get());
			}
		}

	private:

		void OnRequestOutgoingMessage(messages::IMessage& msg,
			messages::DispatchID clientID);

		BaseHandlerRequestInfo* GetRequestInfo(HandlerRequestInfoID requestInfoID);

		HandlerRequestInfoID GetRequestInfoID(messages::DispatchID clientID,
			RequestID requestID) const
		{
			auto itClient = m_requestClients.find(clientID);
			if (itClient == m_requestClients.end())
			{
				return 0;
			}

			return itClient->second.GetRequestInfoID(requestID);
		}

		BaseHandlerRequestInfo* GetRequestFromCoords(messages::DispatchID clientID,
			RequestID requestID)
		{
			// The function in the call below returns 0 if the request info ID was not found; but
			// IDs start at 1.
			auto requestInfoID = GetRequestInfoID(clientID, requestID);
			return GetRequestInfo(requestInfoID);
		}



		void PurgeRequest(HandlerRequestInfoID requestInfoID);
		void PurgeRequests(ClientInfo& client);

		// Two ways to look up request information: clientID/requestID, or
		// request handler ID
		std::unordered_map<messages::DispatchID, ClientInfo> m_requestClients;
		std::unordered_map<HandlerRequestInfoID, std::shared_ptr<BaseHandlerRequestInfo> > m_requestInfos;
		HandlerRequestInfoID m_freeID{ 1 };

		friend class BaseHandlerRequestInfo;
	};

}