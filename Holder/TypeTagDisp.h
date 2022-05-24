#pragma once

#include "TypeTags.h"

#include <unordered_map>

namespace holder::base::types
{
	class DuplicateTagException { };

	template<typename Subject, typename Object,
		typename ... Satellites>
	class TypeTagsDisp
	{
	public:
		TypeTagsDisp(const TypeTagsDisp&) = delete;

		using DispMember = void (Subject::*)(const Object&, Satellites...);

		TypeTagsDisp()
		{
			// Should be a static member of the subject class
			// the second argument is a disambiguator
			Subject::InitializeTagDispatch(static_cast<const Object*>(nullptr), 
				*this);
		}

		void AddDispatch(const TypeTag& tag,
			DispMember dispMember)
		{
			auto emplResult = m_dispatchTable.emplace(tag, dispMember);

			if (!emplResult.second)
			{
				throw DuplicateTagException();
			}
		}

		bool operator()(Subject* pThis, 
			const TypeTag& tag,
			Object& obj,
			Satellites&&... sat)
		{
			if (!pThis)
			{
				return false;
			}

			auto itObj = m_dispatchTable.find(tag);
			if (itObj == m_dispatchTable.end())
			{
				return false;
			}

			DispMember callPtr = itObj->second;

			pThis->*(callPtr)(obj, std::forward<Satellites>(sat)...);

			return true;
		}

	private:
		std::unordered_map<TypeTag, DispMember>
			m_dispatchTable;
	};

}