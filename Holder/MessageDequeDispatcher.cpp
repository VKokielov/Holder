#include "MessageDequeDispatcher.h"

#include <limits>

namespace impl_ns = holder::messages;
namespace msg_ns = holder::messages;

bool impl_ns::MessageDequeDispatcher::MQDSenderEndpoint::SendMessage(const std::shared_ptr<IMessage>& pMsg)
{
	if (!pMsg
		|| (m_pFilter && !m_pFilter->CanSendMessage(*pMsg)))
	{
		return false;
	}

	MQDEnvelope envelope(pMsg, m_receiverID);
	auto pSharedDispatcher = m_pDispatcher.lock();

	if (pSharedDispatcher)
	{
		pSharedDispatcher->SendMessage(std::move(envelope));
		return true;
	}

	return false;
}

msg_ns::ReceiverID
impl_ns::MessageDequeDispatcher::CreateReceiver(const std::shared_ptr<msg_ns::IMessageListener>& pListener,
	std::shared_ptr<IMessageFilter> pFilter,
	msg_ns::DispatchID dispatchId)
{
	std::unique_lock lkQueue(m_mutexQueue);

	m_receiverMap.emplace(std::piecewise_construct, 
		std::forward_as_tuple(m_freeRcvId), 
		std::forward_as_tuple(m_freeRcvId, dispatchId, pListener, pFilter));

	return 	++m_freeRcvId;;
}

std::shared_ptr<impl_ns::ISenderEndpoint> impl_ns::MessageDequeDispatcher::CreateEndpoint(ReceiverID rcvrId)
{
	// Fetch the filter in case there is one
	std::shared_ptr<IMessageFilter> pFilter;
	std::shared_ptr<ISenderEndpoint> pRet;

	{
		std::unique_lock lkQueue(m_mutexQueue);
		auto itReceiver = m_receiverMap.find(rcvrId);
		if (itReceiver == m_receiverMap.end())
		{
			return pRet;
		}
		pFilter = itReceiver->second.pFilter;
	}

	pRet = std::make_shared<MQDSenderEndpoint>(pFilter, shared_from_this(), rcvrId);

	return pRet;
}

void impl_ns::MessageDequeDispatcher::ProcessMessages(WorkStateDescription& workState)
{
	if (m_localQueue.empty())
	{
		std::unique_lock lkQueue(m_mutexQueue);
		m_localQueue.swap(m_queue);
	}

	size_t msgsProcessed = 0;
	size_t msgsToProcess = workState.msgsToProcess > 0 ? workState.msgsToProcess : std::numeric_limits<size_t>::max();

	while (msgsProcessed < msgsToProcess 
		&& !m_localQueue.empty())
	{
		MQDEnvelope& rEnvelope = m_localQueue.back();

		// Find the receiver
		auto itReceiver = m_receiverMap.find(rEnvelope.GetReceiverID());

		if (itReceiver != m_receiverMap.end())
		{
			itReceiver->second.Dispatch(rEnvelope.GetMessage());
		}
		else if (m_traceLostMessages)
		{
			OnLostMessage(rEnvelope.GetMessage(), rEnvelope.GetReceiverID());
		}
		
		m_localQueue.pop_back();
		++msgsProcessed;
		--m_totalSize;
	}

	workState.msgsRemaining = m_totalSize.load();
}

void impl_ns::MessageDequeDispatcher::SendMessage(MQDEnvelope&& envelope)
{
	std::unique_lock lkQueue(m_mutexQueue);
	
	m_queue.push_front(std::move(envelope));
	++m_totalSize;

	DoSignal();
}

bool impl_ns::MessageDequeDispatcher::RemoveReceiver(ReceiverID rcvId)
{
	std::unique_lock lkQueue(m_mutexQueue);

	auto itReceiver = m_receiverMap.find(rcvId);
	if (itReceiver != m_receiverMap.end())
	{
		m_receiverMap.erase(itReceiver);
	}

	return true;
}

void impl_ns::MessageDequeDispatcher::OnLostMessage(const std::shared_ptr<IMessage>& pMessage,
	ReceiverID rcvId)
{

}