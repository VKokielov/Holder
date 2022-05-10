#include "AppLibrary.h"

namespace impl_ns = holder::base;

bool impl_ns::AppLibrary::RegisterFactory(const char* pAddress,
	std::shared_ptr<IAppObjectFactory> pFactory)
{
	std::unique_lock lk(m_mutex);

	std::string sAddress(pAddress);

	auto emplResult = m_library.emplace(sAddress, pFactory);
	return emplResult.second;
}

bool impl_ns::AppLibrary::FindFactory(const char* pAddress,
	std::shared_ptr<IAppObjectFactory>& pFactory)
{
	std::unique_lock lk(m_mutex);
	std::string sAddress(pAddress);

	auto itFactory = m_library.find(sAddress);

	if (itFactory == m_library.end())
	{
		return false;
	}

	pFactory = itFactory->second;
	return true;
}

impl_ns::AppLibrary& impl_ns::AppLibrary::GetInstance()
{
	static AppLibrary me;
	return me;
}