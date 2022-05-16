#pragma once

#include "IService.h"
#include "Messaging.h"
#include "IProxy.h"
#include "BaseServiceLink.h"

namespace holder::service
{

	
	class BaseProxy : public messages::BaseMessageHandler,
		public IServiceLink
	{
	public:
		BaseProxy(const std::shared_ptr<messages::IMessageDispatcher>& pDispatcher,
			std::shared_ptr<messages::ISenderEndpoint> pEndpoint);

		void OnMessage(const std::shared_ptr<messages::IMessage>& pMsg,
			messages::DispatchID dispatchId) override;
		messages::ReceiverID GetReceiverID() const;

		~BaseProxy();
	protected:
		const std::shared_ptr<messages::ISenderEndpoint>&
			CntPart()
		{
			return m_pCounterpart;
		}
	private:
		std::shared_ptr<messages::ISenderEndpoint> m_pCounterpart;
	};

	


}