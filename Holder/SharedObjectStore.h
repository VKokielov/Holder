#pragma once

#include "SharedObjects.h"
#include "ObjectTree.h"
#include "PathLib.h"
#include "PathFromString.h"
#include "SharedObjectStoreInterfaces.h"

#include <cinttypes>
#include <unordered_map>
#include <shared_mutex>
#include <random>

namespace holder::base
{

	class SharedObjectStore
	{
	private:
		struct StoredObject_
		{
			uint32_t token;
			SharedObjectPtr objPtr;
			lib::NodeID nodeId;

			StoredObject_(uint32_t token_, SharedObjectPtr objPtr_,
				lib::NodeID nodeId_)
				:token(token_),
				objPtr(objPtr_),
				nodeId(nodeId_)
			{ }
		};
	public:
		StoredObjectToken AddObject(const char* path,
			SharedObjectPtr pObject);

		bool RemoveObject(const StoredObjectToken& objToken);
		
		template<typename F>
		bool GetObject(StoredObjectID id, F&& callback) const
		{
			std::shared_lock lk(m_storeMutex);

			auto itObj = m_objStore.find(id);

			if (itObj == m_objStore.end())
			{
				return false;
			}

			return callback(id, itObj->second.objPtr);
		}

		template<typename F>
		bool FindObject(const char* path, F&& callback) const
		{
			std::shared_lock lk(m_storeMutex);

			lib::PathFromString pfs(path);
			lib::NodeFinder<StoredObjectID> nodeFinder;
			
			if (lib::TracePath(m_objTree, pfs, nodeFinder) != lib::NodeResult::OK)
			{
				return false;
			}

			StoredObjectID objID{ 0 };
			if (m_objTree.GetValue(nodeFinder.GetNodeID(), objID) 
				!= lib::NodeResult::OK)
			{
				// Not a leaf
				return false;
			}

			return GetObject(objID, std::forward<F>(callback));
		}

		static SharedObjectStore& GetInstance();
	private:
		SharedObjectStore();

		mutable std::shared_mutex m_storeMutex;

		// Objects
		std::unordered_map<SharedObjectID, StoredObject_> m_objStore;
		StoredObjectID m_nextId{ 0 };
		// Token map
		std::unordered_map<uint32_t, StoredObjectID> m_objTokenMap;

		// Object tree
		lib::ObjectTree<StoredObjectID>   m_objTree;

		// Random number generator
		std::mt19937_64  m_generator;

		// Uniform int distribution for the generation
		std::uniform_int_distribution<uint32_t> m_distrib;

	};
}