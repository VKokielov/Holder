#include "BaseClientObject.h"

namespace impl_ns = holder::service;

holder::messages::ReceiverID impl_ns::BaseClientObject::GetReceiverID() const
{
	return m_receiverID;
}