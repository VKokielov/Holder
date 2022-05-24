#pragma once
#include "IAppObjectFactory.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <cinttypes>
#include <string>

namespace holder::base
{

	class AppLibrary
	{
	public:
		bool RegisterFactory(const char* pAddress,
			std::shared_ptr<IAppObjectFactory> pFactory);

		bool FindFactory(const char* pAddress,
			std::shared_ptr<IAppObjectFactory>& pFactory);
		
		static AppLibrary& GetInstance();

	private:
		std::mutex m_mutex;
		std::unordered_map<std::string, std::shared_ptr<IAppObjectFactory> >
			m_library;
	};




}