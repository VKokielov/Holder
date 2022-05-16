#pragma once

#include "IService.h"
#include "IProxy.h"

namespace holder::stream
{

	class ITextService : public service::IService, 
		public service::IServiceObject
	{

	};

	class ITextServiceProxy : public base::IAppObject
	{
	public:
		virtual void OutputString(const char* pString) = 0;
	};

}