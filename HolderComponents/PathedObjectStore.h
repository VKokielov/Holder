#pragma once

#include <shared_mutex>
#include <memory>
#include "ObjectTree.h"

namespace holder::lib
{
	
	template<typename Object>
	class PathedObjectStore
	{
	public:
		using ObjPtr = std::shared_ptr<Object>;

		bool AddObject(const char* pPath,
			const ObjPtr& obj)
		{

		}

		bool RemoveObject(const char* pPath)
		{

		}

		bool FindObject(const char* pPath, 
			ObjPtr& obj) const
		{

		}

	private:
		std::shared_mutex  m_objMutex;
		ObjectTree<ObjPtr> m_tree;
	};


}