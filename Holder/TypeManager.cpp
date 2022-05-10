#include "TypeInfoObjs.h"

namespace impl_ns = holder::base;

impl_ns::TypeManager& impl_ns::TypeManager::GetInstance()
{
	static TypeManager me;
	return me;
}

impl_ns::TypeID impl_ns::TypeManager::AddTypeID(const char* typeName)
{
	return GetInstance().AddTypeIDInst(typeName);
}

impl_ns::TypeID impl_ns::TypeManager::GetTypeID(const char* typeName)
{
	return GetInstance().GetTypeIDInst(typeName);
}

impl_ns::TypeManager& GetInstance()
{
	static impl_ns::TypeManager inst;
	return inst;
}

impl_ns::TypeID impl_ns::TypeManager::AddTypeIDInst(const char* typeName)
{
	std::unique_lock lk(m_mutex);
	std::string sName(typeName);

	auto emplResult = m_typeIds.emplace(sName, m_freeId);

	if (!emplResult.second)
	{
		throw DuplicateTypeException();
	}

	TypeID newId = m_freeId;
	++m_freeId;
	return newId;
}

impl_ns::TypeID impl_ns::TypeManager::GetTypeIDInst(const char* typeName) const
{
	std::shared_lock lk(m_mutex);
	std::string sName(typeName);

	auto itType = m_typeIds.find(sName);

	if (itType == m_typeIds.end())
	{
		throw DuplicateTypeException();
	}

	return itType->second;
}