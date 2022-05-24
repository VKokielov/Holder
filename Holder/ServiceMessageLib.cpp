#include "ServiceMessageLib.h"
#include "MessageTypeTags.h"

namespace impl_ns = holder::service;
namespace messages = holder::messages;

bool impl_ns::ServiceMessageFilter::CanSendMessage(const messages::IMessage& msg)
{
	return dynamic_cast<const IServiceMessage*>(&msg) != nullptr;
}

holder::base::types::TypeTag impl_ns::DestroyClientMessage::GetTag() const
{
	return holder::base::constants::GetDestroyMessageTag();
}