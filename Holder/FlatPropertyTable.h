#pragma once

#include "IAppObject.h"
#include <cinttypes>
#include <string>

namespace holder::base
{

	using PropertyKey = uint64_t;

	class IFlatPropertyTable : public IAppObject
	{
	public:
		virtual bool GetSignedInteger(PropertyKey key, int64_t& value) const
			= 0;
		virtual bool GetUnsignedInteger(PropertyKey key, uint64_t& value) const
			= 0;
		virtual bool GetBoolean(PropertyKey key, bool& value) const
			= 0;
		virtual bool GetString(PropertyKey key, std::string& value) const
			= 0;
	};


}