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
			static_cast<BaseRequestInfo&>(info).SetState(targetState);
		}
	};


}