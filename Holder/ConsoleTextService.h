#pragma once

#include "ITextService.h"
#include "BaseService.h"
#include "BaseServiceObject.h"
#include "BaseProxy.h"
#include "BaseClientObject.h"
#include "BaseMessageDispatch.h"
#include "MQDExecutor.h"
#include "AppObjectFactoryImpl.h"
#include "TypeTagDisp.h"

namespace holder::stream
{

	class ConsoleTextService;

	class ConsoleTextProxy : public service::BaseProxy,
		public stream::ITextServiceProxy,
		public std::enable_shared_from_this<ConsoleTextProxy>,
		protected messages::BaseMessageDispatch
	{
	public:
		ConsoleTextProxy(const std::shared_ptr<messages::IMessageDispatcher>& pDispatcher,
			std::shared_ptr<messages::ISenderEndpoint> pEndpoint)
			:BaseProxy(pDispatcher, std::move(pEndpoint))
		{

		}

		void OnMessage(const std::shared_ptr<messages::IMessage>& pMsg, messages::DispatchID dispatchId) override;

		std::shared_ptr<BaseMessageHandler>
			GetMyBaseHandlerSharedPtr() override;

		void OutputString(const char* pString) override;

		static service::MessageDispatch<ConsoleTextProxy>& GetMessageDispatchTable();

	private:
		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			TagDispatch& tagDispatchTable)
		{
			BaseProxy::InitializeTagDispatch((const messages::IMessage*)nullptr,
				tagDispatchTable);
		}

		friend class base::types::TypeTagsDisp<ConsoleTextProxy, messages::IMessage,
			messages::DispatchID>;
	};

	class ConsoleTextClient : public service::BaseClientObject
	{
	public:
		ConsoleTextClient(ConsoleTextService& owner,
			 messages::DispatchID clientID, 
			 messages::ReceiverID receiverID,
			 std::shared_ptr<messages::ISenderEndpoint> pCounterpart)
			:BaseClientObject(clientID, receiverID, pCounterpart),
			m_owner(owner)
		{ }

	private:
		ConsoleTextService& m_owner;
	};

	class ConsoleTextService : public service::MQDBaseService<ITextService>,
		public service::BaseServiceObject<ConsoleTextService, ConsoleTextProxy, ConsoleTextClient>,
		public std::enable_shared_from_this<ConsoleTextService>,
		protected messages::BaseMessageDispatch
	{
	private:
		using ServiceBase = service::MQDBaseService<ITextService>;
		using SOBase = service::BaseServiceObject<ConsoleTextService, ConsoleTextProxy, ConsoleTextClient>;

		class ConsoleTextMessage : public service::TypedServiceMessage<ConsoleTextMessage, ConsoleTextClient>
		{
		public:
			ConsoleTextMessage(const char* pText)
				:m_text(pText)
			{ }

			void TypedAct(ConsoleTextClient& client);

		private:
			std::string m_text;
		};

	public:

		void OnMessage(const std::shared_ptr<messages::IMessage>& pMsg, messages::DispatchID dispatchId) override;

		std::shared_ptr < service::IServiceLink >
			CreateProxy(const std::shared_ptr<messages::IMessageDispatcher>& pReceiver) override;

		static service::MessageDispatch<ConsoleTextService>& GetMessageDispatchTable();

	protected:
		void OnCreated() override;
		// enable_shared_from_this() accessors
		std::shared_ptr<MessageDequeDispatcher> GetMyMessageDispatcherSharedPtr() override;
		std::shared_ptr<messages::IMessageListener> GetMyListenerSharedPtr() override;
		std::shared_ptr<base::startup::ITaskStateListener> GetMyTaskStateListenerSharedPtr() override;
	private:
		static service::MessageDispatch<ConsoleTextService> m_tagMessageDispatch;

		ConsoleTextService(const service::IServiceConfiguration& config);

		template<typename TagDispatch>
		static void InitializeTagDispatch(const messages::IMessage*,
			TagDispatch& tagDispatchTable)
		{
			SOBase::InitializeTagDispatch((const messages::IMessage*)nullptr,
				tagDispatchTable);
		}


		friend class base::DefaultAppObjectFactory<ConsoleTextService, service::IService>;
		friend class ConsoleTextProxy;
		friend class ConsoleTextClient;
		friend class base::types::TypeTagsDisp<ConsoleTextService, messages::IMessage,
			messages::DispatchID>;
	};




}