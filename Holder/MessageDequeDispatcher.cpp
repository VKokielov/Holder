#include "MessageDequeDispatcher.h"

#include <limits>

namespace impl_ns = holder::messages;
namespace msg_ns = holder::messages;

/*
bool impl_ns::MessageDequeDispatcher::MQDSenderEndpoint::SendMessage(const std::shared_ptr<IMessage>& pMsg)
{

	MQDEnvelope envelope(pMsg, m_receiverID);
	auto pSharedDispatcher = m_pDispatcher.lock();

	if (pSharedDispatcher)
	{
		pSharedDispatcher->SendMessage(std::move(envelope));
		return true;
	}

	return false;
}
*/

void impl_ns::MessageDequeDispatcher::SendMessage(ReceiverID rcvrId, std::shared_ptr<IMessage> pMessage)
{
	MQDMessageEnvelope envelope(rcvrId, std::move(pMessage));
	++m_totalSize;
	PostMessage(std::move(envelope), m_queue);
}

msg_ns::ReceiverID
impl_ns::MessageDequeDispatcher::CreateReceiver(const std::shared_ptr<msg_ns::IMessageListener>& pListener,
	std::shared_ptr<IMessageFilter> pFilter,
	msg_ns::DispatchID dispatchId)
{
	if (!pListener)
	{
		throw MessageException();
	}

	ReceiverID newReceiverID = m_freeRcvId++;
	MQDCreateReceiverEnvelope envelope(newReceiverID, pListener, pFilter, dispatchId);
	PostMessage(std::move(envelope), m_createQueue);
	return newReceiverID;
}


void impl_ns::MessageDequeDispatcher::RemoveReceiver(ReceiverID rcvId)
{
	MQDRemoveReceiverEnvelope envelope(rcvId);
	PostMessage(std::move(envelope), m_removeQueue);
}

/*
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

	pRet = std::make_shared<MQDSenderEndpoint>(pFilter, GetMyMessageDispatcherSharedPtr(), rcvrId);

	return pRet;
}
*/


void impl_ns::MessageDequeDispatcher::MQDMessageEnvelope::Act(MessageDequeDispatcher& dispatcher)
{
	auto itReceiver = dispatcher.m_receiverMap.find(GetReceiverID());
	if (itReceiver != dispatcher.m_receiverMap.end())
	{
		itReceiver->second.Dispatch(m_pMessage);
	}
	else
	{
		dispatcher.OnLostMessage(m_pMessage, GetReceiverID());
	}
}

void impl_ns::MessageDequeDispatcher::MQDCreateReceiverEnvelope::Act(MessageDequeDispatcher& dispatcher)
{
	dispatcher.m_receiverMap.emplace(std::piecewise_construct,
		std::forward_as_tuple(GetReceiverID()),
		std::forward_as_tuple(GetReceiverID(), m_dispatchId, m_pListener, m_pFilter));

	m_pListener->OnReceiverReady(m_dispatchId);
}

void impl_ns::MessageDequeDispatcher::MQDRemoveReceiverEnvelope::Act(MessageDequeDispatcher& dispatcher)
{
	auto itReceiver = dispatcher.m_receiverMap.find(GetReceiverID());
	if (itReceiver != dispatcher.m_receiverMap.end())
	{
		dispatcher.m_receiverMap.erase(itReceiver);
	}
}

void impl_ns::MessageDequeDispatcher::ProcessMessages(WorkStateDescription& workState)
{
	{
		std::unique_lock lkQueue(m_mutexQueue);
		if (m_localQueue.empty())
		{
			m_localQueue.swap(m_queue);
		}

		m_localCreateQueue.swap(m_createQueue);
		m_localRemoveQueue.swap(m_removeQueue);
	}

	// Process all create and then all remove requests
	while (!m_localCreateQueue.empty())
	{
		m_localCreateQueue.back().Act(*this);
		m_localCreateQueue.pop_back();
	}

	while (!m_localRemoveQueue.empty())
	{
		m_localRemoveQueue.back().Act(*this);
		m_localRemoveQueue.pop_back();
	}

	size_t msgsProcessed = 0;
	size_t msgsToProcess = workState.msgsToProcess > 0 ? workState.msgsToProcess : std::numeric_limits<size_t>::max();

	while (msgsProcessed < msgsToProcess 
		&& !m_localQueue.empty())
	{
		MQDMessageEnvelope& rEnvelope = m_localQueue.back();
		rEnvelope.Act(*this);
		
		m_localQueue.pop_back();
		++msgsProcessed;
		--m_totalSize;
	}

	workState.msgsRemaining = m_totalSize.load();
}


void impl_ns::MessageDequeDispatcher::OnLostMessage(const std::shared_ptr<IMessage>& pMessage,
	ReceiverID rcvId)
{

}