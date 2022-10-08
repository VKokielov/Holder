#pragma once

#include "AppInterface.h"
#include "PathedObjectStore.h"
#include "SingletonBase.h"

namespace holder::base
{

	class ObjectManager : public SingletonBase
	{
	public:

	private:

		lib::PathedObjectStore<IAppObject>
			m_objectStore;

		
	};

}