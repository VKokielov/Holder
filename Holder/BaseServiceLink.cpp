#include "BaseServiceLink.h"

namespace impl_ns = holder::service;
namespace messages = holder::messages;

bool impl_ns::ServiceMessageFilter::CanSendMessage(const messages::IMessage& msg)
{
	return dynamic_cast<const IServiceMessage*>(&msg) != nullptr;
}

bool impl_ns::DestroyClientMessage::IsDestroyMessage()
{
	return true;
}

void impl_ns::DestroyClientMessage::Act(IServiceLink& object)
{

}
