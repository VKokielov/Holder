#include "TypeTags.h"

namespace impl_ns = holder::base::types;

impl_ns::TypeTagManager& impl_ns::TypeTagManager::GetInstance()
{
	static TypeTagManager me;
	return me;
}


impl_ns::TypeTag impl_ns::TypeTagManager::GetType(const char* typeName)
{
	return GetInstance().GetTypeInst(typeName);
}

impl_ns::TypeTag impl_ns::TypeTagManager::GetTypeInst(const char* typeName)
{
	std::string sName(typeName);

	{
		// First try shared
		std::shared_lock lk(m_mutex);
		auto itType = m_typeMap.find(typeName);
		if (itType != m_typeMap.end())
		{
			return itType->second;
		}
	}

	std::unique_lock lk(m_mutex);
	auto itType = m_typeMap.find(typeName);
	// check again
	if (itType != m_typeMap.end())
	{
		return itType->second;
	}

	// No?  Add
	TypeID newTypeID = m_freeId++;
	m_typeMap.emplace(typeName, TypeTag(newTypeID));

	return TypeTag(newTypeID);

}