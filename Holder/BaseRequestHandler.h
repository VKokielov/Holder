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
		BaseRequestHandler* pOwner;
		HandlerRequestInfoID requestInfoID;
		messages::DispatchID clientID;
		RequestID requestID;
		std::weak_ptr<messages::ISenderEndpoint> pRemoteEndpoint;
	};

	class BaseHandlerRequestInfo
	{
	public:
		// The public interface is to be used by the class deriving from the request handler
		void CompleteRequest(bool success,
							std::shared_ptr<base::IAppObject> pResult);
		
		void Purge();

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
	protected:
		
		BaseHandlerRequestInfo(const BaseHandlerRIInitializer& bhInitializer,
			bool autoPurge)
			:m_owner(*bhInitializer.pOwner),
			m_requestInfoID(bhInitializer.requestInfoID),
			m_clientID(bhInitializer.clientID),
			m_requestID(bhInitializer.requestID),
			m_autoPurge(autoPurge)
		{ }

		virtual void OnRequestCancelled();
		virtual void OnRequestComplete(bool success,
			const std::shared_ptr<base::IAppObject>& requestResult);

	private:
		void CancelRequest();
		void SetCompletion(bool success,
			std::shared_ptr<base::IAppObject>& requestResult);

		// Arguments
		// Purge on completion?

		BaseRequestHandler& m_owner;
		bool m_autoPurge{ false };

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

			template<typename Msg, typename...Args>
			void SendRequestMessage(RequestID requestID, Args&&...args)
			{
				messages::SendMessage<Msg>(m_pRemoteEndpoint, 
					requestID,
					std::forward<Args>(args)...);
			}
		private:
			std::unordered_map<RequestID,
				HandlerRequestInfoID> m_requestMap;
			// Other information
			std::shared_ptr<messages::ISenderEndpoint>
				m_pRemoteEndpoint;
		};

	public:

		void CancelRequest(RequestID requestID, messages::DispatchID clientID);


	protected:

		void AddClient(messages::DispatchID clientID);
		void RemoveClient(messages::DispatchID clientID);

		void PurgeRequest(HandlerRequestInfoID requestInfoID);

		template<typename HandlerRequestInfoType,
				 typename ... Args>
		HandlerRequestInfoID
			CreateRequest(messages::DispatchID clientID,
				RequestID requestID, 
				Args&& ... args)
		{
			static_assert(std::is_base_of_v<BaseHandlerRequestInfo, HandlerRequestInfoType>,
				"Your HandlerRequestInfo class must derive from BaseHandlerRequestInfo");
			std::shared_ptr<HandlerRequestInfoType> pRet{};

			// Make it harder to hack
			if (m_freeID == 0) 
			{
				return pRet;
			}

			auto itClient = m_requestClients.find(clientID);

			if (itClient == m_requestClients.end())
			{
				return pRet;
			}

			// A client was found.  Now check that the requestID is NOT already
			// in the map.  If it is, send a fail message.  This is needed for completeness.
			// We can assume nothing about the code on the other side.

			// The create failure message will, for the client, refer to an existing
			// request ID.  Since the request IDs are picked by the client, this should
			// never happen unless there is a bug in the client code

			if (itClient->second.HasRequestID(requestID))
			{
				itClient->second.SendCreateFailure(requestID);
				return pRet;
			}

			// Create the request object and add it to our data structure
			pRet = std::make_shared<HandlerRequestInfoType>(requestID, clientID,
				std::forward<Args>(args)...);

			itClient->second.AddRequest(pRet);
			return pRet;
		}
		
		void IterateIncompleteRequests();

		// This function is called when m_notifyHangingRequests is true
		// and a client goes out of commission, or when IterateIncompleteRequests()
		// above is called.

		virtual void OnIncompleteRequest(const BaseHandlerRequestInfo& request);

	private:
		void OnRequestOutgoingMessage(messages::IMessage& msg,
			messages::DispatchID clientID);

		HandlerRequestInfoID GetRequestInfoID(messages::DispatchID,
			RequestID requestID) const;

		BaseHandlerRequestInfo* GetRequestInfo(HandlerRequestInfoID requestInfoID);

		BaseHandlerRequestInfo* GetRequestFromCoords(messages::DispatchID clientID,
			RequestID requestID)
		{
			// This function returns 0 if the request info ID was not found; but
			// IDs start at 1.

			auto requestInfoID = GetRequestInfoID(clientID, requestID);
			return GetRequestInfo(requestInfoID);
		}

		void IterateClientRequests(ClientInfo& client,
			bool purgeRequests, 
			bool raiseIncomplete);

		// Arguments
		bool m_notifyHangingRequests{ false };

		// Two ways to look up request information: clientID/requestID, or
		// request handler ID
		std::unordered_map<messages::DispatchID, ClientInfo> m_requestClients;
		std::unordered_map<HandlerRequestInfoID, std::unique_ptr<BaseHandlerRequestInfo> > m_requestHandlers;
		HandlerRequestInfoID m_freeID{ 1 };
	};

}