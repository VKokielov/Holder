#include "BaseRequestHandler.h"

namespace impl_ns = holder::reqresp;

bool impl_ns::BaseRequestHandler::ClientInfo::AddRequest(RequestID requestID, HandlerRequestInfoID requestInfoID)
{
	auto emplResult = m_requestMap.emplace(requestID, requestInfoID);
	return emplResult.second;
}
bool impl_ns::BaseRequestHandler::ClientInfo::RemoveRequest(RequestID requestID)
{
	auto itRequest = m_requestMap.find(requestID);

	if (itRequest == m_requestMap.end())
	{
		return false;
	}

	m_requestMap.erase(itRequest);
	return true;
}

bool impl_ns::BaseRequestHandler::ClientInfo::HasRequestID(RequestID requestID) const
{
	return m_requestMap.count(requestID) > 0;
}

void impl_ns::BaseRequestHandler::AddClient(messages::DispatchID clientID)
{

}

void impl_ns::BaseRequestHandler::RemoveClient(messages::DispatchID clientID)
{
	auto itClient = m_requestClients.find(clientID);

	if (itClient == m_requestClients.end())
	{
		return;
	}

	ClientInfo& clientInfo = itClient->second;

	IterateClientRequests(clientInfo, true, m_notifyHangingRequests);

	m_requestClients.erase(clientID);
}

void impl_ns::BaseRequestHandler::IterateClientRequests(ClientInfo& client, 
	bool purgeRequests,
	bool raiseIncomplete)
{

	std::vector<HandlerRequestInfoID> toPurge;

	auto raiseIncomplete = [this, &toPurge, purgeRequests, raiseIncomplete](RequestID requestID, HandlerRequestInfoID requestInfoID)
	{
		// Get the request
		BaseHandlerRequestInfo* pRequestInfo = this->GetRequestInfo(requestInfoID);

		if (raiseIncomplete)
		{
			this->OnIncompleteRequest(*pRequestInfo);
		}

		if (purgeRequests)
		{
			toPurge.push_back(requestInfoID);
		}

		return RequestIterCommand::Continue;
	};

	client.IterateRequests(raiseIncomplete);

	for (HandlerRequestInfoID idToPurge : toPurge)
	{
		PurgeRequest(idToPurge);
	}
}

void impl_ns::BaseRequestHandler::OnRequestOutgoingMessage(messages::IMessage& msg,
	messages::DispatchID clientID)
{
	static_cast<IRequestOutgoingMessage&>(msg).Act(*this, clientID);
}