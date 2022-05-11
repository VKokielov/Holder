#pragma once

#include "IService.h"
#include "IProxy.h"

namespace holder::stream
{

	class ITextService : public service::IService, service::IServiceObject
	{

	};

	class ITextServiceProxy : public service::IServiceLink
	{
	public:
		virtual void OutputString(const char* pString) = 0;
	};

}