#pragma once

#include "IService.h"
#include "CloneHelper.h"

#include <string>
#include <memory>

namespace holder::service
{
	class BaseServiceConfiguration : public IServiceConfiguration,
		private lib::CloneHelper<BaseServiceConfiguration>
	{
	public:
		BaseServiceConfiguration(const BaseServiceConfiguration& config);

		const char* GetServicePath() const override;
		const char* GetServiceTypeName() const override;
		const char* GetServiceThreadName() const override;
		bool TraceLostMessages() const override;
		const IServiceInstanceConfiguration* GetInstanceConfiguration() const override;
		BaseServiceConfiguration* Clone() const override;

		BaseServiceConfiguration(const char* pServicePath,
			const char* pServiceTypeName,
			const char* pServiceThreadName,
			bool traceLostMessages,
			std::shared_ptr<IServiceInstanceConfiguration> pInstanceConfig)
			:m_servicePath(pServicePath),
			m_serviceTypeName(pServiceTypeName),
			m_serviceThreadName(pServiceThreadName),
			m_traceLostMessages(traceLostMessages),
			m_pInstanceConfig(pInstanceConfig)
		{ }

	private:
		std::string m_servicePath;
		std::string m_serviceTypeName;
		std::string m_serviceThreadName;
		bool m_traceLostMessages;
		std::shared_ptr<IServiceInstanceConfiguration>  m_pInstanceConfig;
	};
}