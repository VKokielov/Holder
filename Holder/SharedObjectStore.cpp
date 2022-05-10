#include "SharedObjectStore.h"

#include <limits>

namespace impl_ns = holder::base;

impl_ns::SharedObjectStore::SharedObjectStore()
	:m_distrib(1, std::numeric_limits<uint32_t>::max())
{
	std::random_device device;
	m_generator.seed(device());
}

impl_ns::SharedObjectStore& impl_ns::SharedObjectStore::GetInstance()
{
	static SharedObjectStore me;
	return me;
}

holder::base::StoredObjectToken impl_ns::SharedObjectStore::AddObject(const char* path,
	holder::base::SharedObjectPtr pObject)
{
	std::unique_lock lk(m_storeMutex);

	// Prevent an infinite loop on tokens
	if (m_objTokenMap.size() == std::numeric_limits<uint32_t>::max())
	{
		return g_invalidObjToken;
	}

	StoredObjectToken tokObj;

	lib::PathFromString objPath(path);
	lib::NodeAdder<StoredObjectID> nodeAdder(true);

	lib::NodeResult addResult =
		lib::TracePath(m_objTree, objPath, nodeAdder);

	if (addResult != lib::NodeResult::OK)
	{
		return g_invalidObjToken;
	}

	// Successfully added.  Assign an ID and a token
	StoredObjectID objId = m_nextId++;

	m_objTree.SetValue(nodeAdder.GetNodeID(), objId);

	bool genToken{ true };

	uint32_t objToken;
	while (genToken)
	{
		objToken = m_distrib(m_generator);
		genToken = m_objTokenMap.count(objToken) != 0;
	}

	m_objTokenMap.emplace(objToken, objId);
	tokObj.token = objToken;

	m_objStore.emplace(objId, StoredObject_(objToken, pObject, nodeAdder.GetNodeID()));

	return tokObj;
}

bool impl_ns::SharedObjectStore::RemoveObject(const StoredObjectToken& objToken)
{
	std::unique_lock lk(m_storeMutex);
	
	auto itObjToken = m_objTokenMap.find(objToken.token);
	if (itObjToken == m_objTokenMap.end())
	{
		return false;
	}

	auto itObj = m_objStore.find(itObjToken->second);

	// Remove from tree updating the parent directory
	m_objTree.RemoveNode(itObj->second.nodeId, true);
	
	// Remove from the object store and from the tokens
	m_objStore.erase(itObj);
	m_objTokenMap.erase(itObjToken);

	return true;
}
