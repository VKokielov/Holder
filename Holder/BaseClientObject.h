#pragma once

#include "BaseServiceLink.h"

namespace holder::service
{

	class BaseClientObject : public IServiceLink
	{
	public:
		messages::ReceiverID GetReceiverID() const override;
	
	protected:
		BaseClientObject(messages::DispatchID clientID, messages::ReceiverID receiverID, 
			std::shared_ptr<messages::ISenderEndpoint> pCounterpart)
			:m_clientID(clientID),
			m_receiverID(receiverID),
			m_pCounterpart(pCounterpart)
		{ }

		messages::DispatchID GetClientID_() const { return m_clientID; }
		messages::ReceiverID GetReceiverID_() const { return m_receiverID; }
		const std::shared_ptr<messages::ISenderEndpoint>& GetCounterpart() const { return m_pCounterpart; }

	private:

		messages::DispatchID m_clientID;
		messages::ReceiverID m_receiverID;
		std::shared_ptr<messages::ISenderEndpoint> m_pCounterpart;
	};


}