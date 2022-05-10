#pragma once

namespace holder::base
{

	class IAppObject
	{
	public:
		virtual ~IAppObject() = default;
	};

	class IAppArgument : public IAppObject
	{
	public:
		virtual IAppArgument* Clone() const = 0;

		virtual ~IAppArgument() = default;
	};

}