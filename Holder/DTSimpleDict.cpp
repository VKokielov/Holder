#include "DTSimpleDict.h"
namespace impl_ns = holder::data::simple;

bool impl_ns::Dictionary::IsEmpty() const
{
	return m_dictionary.empty();
}

holder::data::BaseDatumType impl_ns::Dictionary::GetDatumType() const
{
	return BaseDatumType::Dictionary;
}

bool impl_ns::Dictionary::IsImmutable() const
{
	return false;
}

bool impl_ns::Dictionary::HasEntry(const char* pKey) const
{
	std::string strKey{ pKey };

	return m_dictionary.count(strKey) > 0;
}

bool impl_ns::Dictionary::GetEntry(const char* pKey, std::shared_ptr<IDatum>& rChild) const
{
	std::string strKey(pKey);

	auto itEntry = m_dictionary.find(strKey);
	if (itEntry != m_dictionary.end())
	{
		rChild = itEntry->second;
		return true;
	}

	return false;
}

bool impl_ns::Dictionary::Iterate(holder::data::IDictCallback& rCallback) const
{
	for (const auto& rEntry : m_dictionary)
	{
		if (!rCallback.OnEntry(rEntry.first.c_str(), rEntry.second))
		{
			return false;
		}
	}

	return true;
}

bool impl_ns::Dictionary::SetEntry(const char* pKey, const std::shared_ptr<IDatum>& rChild)
{
	std::string strKey{ pKey };

	auto emplResult = m_dictionary.emplace(strKey, rChild);
	if (!emplResult.second)
	{
		emplResult.first->second = rChild;
		return false;
	}

	return true;
}