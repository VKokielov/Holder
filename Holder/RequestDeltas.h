#pragma once

#include "BaseRequestInfo.h"

namespace holder::reqresp
{
	// Request deltas are messages sent to requesters from request servicers
	
	// Standard delta to set the state
	template<RequestState targetState>
	class RequestSetStateDelta
	{
	public:
		void operator()(IRequestInfo& info)
		{
			auto updater = static_cast<BaseRequestInfo&>(info).GetUpdater();
			updater.SetNewState(targetState);
			updater();
		}
	};

	class RequestCompleteDelta
	{
	public:
		RequestCompleteDelta(RequestCompleteDelta&& other) = default;

		RequestCompleteDelta(bool success, std::shared_ptr<base::IAppObject> pResult)
		{ }

		void operator()(IRequestInfo& info)
		{
			auto updater = static_cast<BaseRequestInfo&>(info).GetUpdater();
			updater.SetNewState(holder::reqresp::RequestState::Completed);
			if (m_pResult)
			{
				updater.SetResult(m_pResult);
			}

			updater();
		}

	private:
		bool m_success;
		std::shared_ptr<base::IAppObject> m_pResult;
	};


}