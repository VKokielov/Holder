#include "BaseServiceConfiguration.h"

namespace impl_ns = holder::service;

impl_ns::BaseServiceConfiguration::BaseServiceConfiguration(const BaseServiceConfiguration& other)
	:m_servicePath(other.m_servicePath),
	m_serviceTypeName(other.m_serviceTypeName),
	m_serviceThreadName(other.m_serviceThreadName),
	m_traceLostMessages(other.m_traceLostMessages),
	m_pInstanceConfig(
		(bool)m_pInstanceConfig ?
		static_cast<IServiceInstanceConfiguration*>(other.m_pInstanceConfig->Clone()) 
		: nullptr)
{

}

const char* impl_ns::BaseServiceConfiguration::GetServicePath() const
{
	return m_servicePath.c_str();
}
const char* impl_ns::BaseServiceConfiguration::GetServiceTypeName() const
{
	return m_serviceTypeName.c_str();
}
const char* impl_ns::BaseServiceConfiguration::GetServiceThreadName() const
{
	return m_serviceThreadName.c_str();
}
bool impl_ns::BaseServiceConfiguration::TraceLostMessages() const
{
	return m_traceLostMessages;
}
const impl_ns::IServiceInstanceConfiguration* impl_ns::BaseServiceConfiguration::GetInstanceConfiguration() const
{
	return m_pInstanceConfig.get();
}

impl_ns::BaseServiceConfiguration* impl_ns::BaseServiceConfiguration::Clone() const
{
	// Base class
	return CloneHelper<BaseServiceConfiguration>::Clone_();
}