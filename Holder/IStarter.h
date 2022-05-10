#pragma once

#include "IAppObject.h"

namespace holder::service
{
	class IStarterArgument : public base::IAppArgument
	{
	public:
		virtual size_t GetArgCount() const = 0;
		virtual const char* GetArgument(size_t index) const = 0;
	};

	class IServiceStarter : public base::IAppObject
	{
	public:
		virtual bool Start() = 0;
		virtual bool Run() = 0;
	};

}