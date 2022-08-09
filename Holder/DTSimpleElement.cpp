#include "DTSimpleElement.h"

namespace impl_ns = holder::data::simple;

holder::data::BaseDatumType impl_ns::Element::GetDatumType() const
{
	return BaseDatumType::Element;
}

bool impl_ns::Element::IsImmutable() const
{
	return false;
}

template<typename T>
bool impl_ns::Element::GenericGet(T& dest) const
{
	auto* pval = std::get_if<T>(&m_data);
	if (pval)
	{
		dest = *pval;
		return true;
	}

	return false;
}

template<typename T>
void impl_ns::Element::GenericSet(T&& val)
{
	using TBase = std::remove_reference_t<T>;

	auto pval = std::get_if<TBase>(&m_data);

	if (pval)
	{
		*pval = val;
	}
	else
	{
		m_data.emplace<TBase>(val);
	}
}

holder::data::ElementType impl_ns::Element::GetElementType() const 
{
	return IndexToType(m_data.index());
}

bool impl_ns::Element::Get(bool& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(bool datum) 
{
	GenericSet(datum);
}

bool impl_ns::Element::Get(int8_t& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(int8_t datum) 
{
	GenericSet(datum);
}

bool impl_ns::Element::Get(uint8_t& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(uint8_t datum) 
{
	GenericSet(datum);
}

bool impl_ns::Element::Get(int16_t& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(int16_t datum) 
{
	GenericSet(datum);
}
bool impl_ns::Element::Get(uint16_t& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(uint16_t datum) 
{
	GenericSet(datum);
}

bool impl_ns::Element::Get(int32_t& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(int32_t datum) 
{
	GenericSet(datum);
}

bool impl_ns::Element::Get(uint32_t& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(uint32_t datum) 
{
	GenericSet(datum);
}
bool impl_ns::Element::Get(int64_t& rDatum) const
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(int64_t datum)
{
	GenericSet(datum);
}
bool impl_ns::Element::Get(uint64_t& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(uint64_t datum) 
{
	GenericSet(datum);
}

bool impl_ns::Element::Get(float& rDatum) const 
{
	return GenericGet(rDatum);
}

void impl_ns::Element::Set(float datum) 
{
	GenericSet(datum);
}

bool impl_ns::Element::Get(double& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(double datum) 
{
	GenericSet(datum);
}

bool impl_ns::Element::Get(std::string& rDatum) const 
{
	return GenericGet(rDatum);
}
void impl_ns::Element::Set(const char* datum) 
{
	auto pstring = std::get_if<StringIndex>(&m_data);
	if (pstring)
	{
		pstring->assign(datum);
	}
	else
	{
		m_data.emplace<std::string>(datum);
	}
}

void impl_ns::Element::Clear()
{
	m_data.emplace<NoneType>();
}