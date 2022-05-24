#pragma once

#include "IService.h"
#include "Messaging.h"
#include "IProxy.h"
#include "ServiceMessageLib.h"
#include "MessageTypeTags.h"
#include "TypeTagDisp.h"

namespace holder::service
{

	
	class BaseProxy : public messages::BaseMessageHandler,
		public IServiceLink
	{
	public:
		BaseProxy(const std::shared_ptr<messages::IMessageDispatcher>& pDispatcher,
			std::shared_ptr<messages::ISenderEndpoint> pEndpoint);

		messages::ReceiverID GetReceiverID() const;

		~BaseProxy();
	protected:

		const std::shared_ptr<messages::ISenderEndpoint>&
			CntPart()
		{
			return m_pCounterpart;
		}

		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			                              TagDispatch& tagDispatchTable)
		{ 
			tagDispatchTable.AddDispatch(base::constants::GetServiceMessageTag(),
				&BaseProxy::OnServiceMessage);
		}

	private:

		void OnServiceMessage(messages::IMessage& msg,
			messages::DispatchID dispatchID);

		std::shared_ptr<messages::ISenderEndpoint> m_pCounterpart;
	};

	


}