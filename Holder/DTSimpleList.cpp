#include "DTSimpleList.h"
namespace impl_ns = holder::data::simple;

bool impl_ns::List::IsEmpty() const
{
	return m_list.empty();
}

holder::data::BaseDatumType impl_ns::List::GetDatumType() const 
{
	return BaseDatumType::List;
}

bool impl_ns::List::IsImmutable() const
{
	return false;
}

size_t impl_ns::List::GetLength() const
{
	return m_list.size();
}
bool impl_ns::List::GetEntry(size_t idx, std::shared_ptr<IDatum>& rChild) const
{
	if (idx < m_list.size())
	{
		rChild = m_list[idx];
		return true;
	}

	return false;
}

bool impl_ns::List::GetRange(std::vector<std::shared_ptr<IDatum> >& rSequence,
	size_t startIdx,
	size_t endIdx) const 
{
	if (endIdx == LIST_NPOS)
	{
		endIdx = m_list.size();
	}

	if (startIdx >= m_list.size() || endIdx > m_list.size())
	{
		return false;
	}

	size_t nAdded{ 0 };
	for (size_t i = startIdx; i < endIdx; ++i)
	{
		rSequence.emplace_back(m_list[i]);
		++nAdded;
	}

	return nAdded != 0;
	
}
bool impl_ns::List::InsertEntry(const std::shared_ptr<IDatum>& pEntry,
	size_t indexBefore) 
{
	if (indexBefore == LIST_NPOS)
	{
		indexBefore = m_list.size();
	}

	if (indexBefore > m_list.size())
	{
		return false;
	}

	m_list.emplace(m_list.begin() + indexBefore, pEntry);
	return true;
}

bool impl_ns::List::SetEntry(const std::shared_ptr<IDatum>& pEntry,
	size_t indexAt) 
{
	if (indexAt >= m_list.size())
	{
		return false;
	}

	m_list[indexAt] = pEntry;
	return true;
}