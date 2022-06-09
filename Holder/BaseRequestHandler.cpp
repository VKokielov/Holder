#include "BaseRequestHandler.h"
#include "BaseRequestMessages.h"
#include "RequestDeltas.h"

namespace impl_ns = holder::reqresp;

// BaseHandlerRequestInfo

impl_ns::BaseHandlerRequestInfo::~BaseHandlerRequestInfo()
{
	auto pOwner = m_owner.lock();
	if (pOwner)
	{
		pOwner->PurgeRequest(m_requestInfoID);
	}
}
void impl_ns::BaseHandlerRequestInfo::CompleteRequest(bool success,
	std::shared_ptr<holder::base::IAppObject> pResult)
{
	if (m_reqState != HandlerRequestState::Active)
	{
		return;
	}

	m_reqState = HandlerRequestState::Complete;

	RequestCompleteDelta rcDelta(success, pResult);
	SendRequestUpdate(std::move(rcDelta));

	OnRequestComplete(success, pResult);
	
	m_success = success;
	m_pResult = pResult;

	auto pOwner = m_owner.lock();

	if (pOwner)
	{
		pOwner->PurgeRequest(m_requestInfoID);
	}
}

void impl_ns::BaseHandlerRequestInfo::SetCancel()
{
	if (m_reqState != HandlerRequestState::Active)
	{
		return;
	}

	m_reqState = HandlerRequestState::Cancelled;
	
	RequestSetStateDelta<RequestState::CancelAcknowledged> cancelAckDelta;
	SendRequestUpdate(std::move(cancelAckDelta));

	OnRequestCancelled();

	auto pOwner = m_owner.lock();
	if (pOwner)
	{
		pOwner->PurgeRequest(m_requestInfoID);
	}
}

// default callbacks
void impl_ns::BaseHandlerRequestInfo::OnRequestCancelled()
{

}
void impl_ns::BaseHandlerRequestInfo::OnRequestComplete(bool success,
	const std::shared_ptr<holder::base::IAppObject>& requestResult)
{

}
void impl_ns::BaseHandlerRequestInfo::OnRequestOrphaned()
{

}

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

// *BaseRequestHandler*
bool impl_ns::BaseRequestHandler::ClientInfo::HasRequestID(RequestID requestID) const
{
	return m_requestMap.count(requestID) > 0;
}

impl_ns::HandlerRequestInfoID impl_ns::BaseRequestHandler::ClientInfo::GetRequestInfoID(
	impl_ns::RequestID requestID) const
{
	auto itRequest = m_requestMap.find(requestID);
	if (itRequest == m_requestMap.end())
	{
		return 0;
	}

	return itRequest->second;
}


void impl_ns::BaseRequestHandler::CancelRequest(impl_ns::RequestID requestID, 
	holder::messages::DispatchID clientID)
{
	// Find the request, set it to cancel (triggering a cancel ack)
	BaseHandlerRequestInfo* pRequest = GetRequestFromCoords(clientID, requestID);
	if (pRequest)
	{
		pRequest->SetCancel();
	}
}

void impl_ns::BaseRequestHandler::AddClient(holder::messages::DispatchID clientID,
	std::shared_ptr<holder::messages::ISenderEndpoint> pRemoteEndpoint)
{
	m_requestClients.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(clientID),
		std::forward_as_tuple(std::move(pRemoteEndpoint))
	);
}

void impl_ns::BaseRequestHandler::RemoveClient(holder::messages::DispatchID clientID)
{
	auto itClient = m_requestClients.find(clientID);

	if (itClient == m_requestClients.end())
	{
		return;
	}

	ClientInfo& clientInfo = itClient->second;

	PurgeRequests(clientInfo);

	m_requestClients.erase(clientID);
}

void impl_ns::BaseRequestHandler::PurgeRequests(ClientInfo& client)
{
	std::vector<HandlerRequestInfoID> toPurge;

	auto purgeRequests = [this, &toPurge](RequestID requestID, HandlerRequestInfoID requestInfoID)
	{
		// Get the request
		toPurge.push_back(requestInfoID);
		return RequestIterCommand::Continue;
	};

	client.IterateRequests(purgeRequests);

	for (HandlerRequestInfoID idToPurge : toPurge)
	{
		PurgeRequest(idToPurge);
	}
}

void impl_ns::BaseRequestHandler::OnRequestOutgoingMessage(holder::messages::IMessage& msg,
	holder::messages::DispatchID clientID)
{
	static_cast<IRequestOutgoingMessage&>(msg).Act(*this, clientID);
}

impl_ns::BaseHandlerRequestInfo* impl_ns::BaseRequestHandler::GetRequestInfo(HandlerRequestInfoID requestInfoID)
{
	auto itRequestInfo = m_requestInfos.find(requestInfoID);

	if (itRequestInfo == m_requestInfos.end())
	{
		return nullptr;
	}

	return itRequestInfo->second.get();
}
void impl_ns::BaseRequestHandler::PurgeRequest(HandlerRequestInfoID requestInfoID)
{
	// Purge the request
	auto itRequest = m_requestInfos.find(requestInfoID);

	if (itRequest == m_requestInfos.end())
	{
		return;
	}

	auto itClient = m_requestClients.find(itRequest->second->GetClientID());
	if (itClient != m_requestClients.end())
	{
		itClient->second.RemoveRequest(itRequest->second->GetRequestID());
	}

	// Inform the request that it is being orphaned
	itRequest->second->OnRequestOrphaned();

	m_requestInfos.erase(itRequest);
}