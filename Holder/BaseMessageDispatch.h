#pragma once

#include "Messaging.h"

#include <memory>


namespace holder::messages
{

	// This class is designed for protected inheritance, and implements only one
	// real function: a helper to dispatch messages using a type tag dispatch

	// It also implements a single callback, for the case when a message could not be
	// dispatched

	class BaseMessageDispatch
	{
	protected:
		BaseMessageDispatch() = default;

		virtual void OnUnknownMessage(const std::shared_ptr<messages::IMessage>& pMsg,
			messages::DispatchID dispatchId) { }

		template<typename Derived>
		void DispatchMessage(const std::shared_ptr<messages::IMessage>& pMsg,
			messages::DispatchID dispatchID)
		{
			// Helper function to dispatch a message using the tag dispatch table
			// in the derived class

			Derived* pThisDerived = static_cast<Derived*>(this);

			auto& dispTable = Derived::g_tagMessageDispatch;

			if (!dispTable(pThisDerived, pMsg->GetTag(),
				pMsg.get(), dispatchID))
			{
				OnUnknownMessage(pMsg, dispatchID);
			}

		}
	};

}