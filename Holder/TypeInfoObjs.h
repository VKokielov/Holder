#pragma once

#include "TypeID.h"

#include <shared_mutex>
#include <unordered_map>
#include <string>

namespace holder::base
{
	class DuplicateTypeException { };

	class TypeNotFoundException { };

	class InvalidObjTypeException { };

	class TypeManager
	{
	public:
		// AddTypeID and GetTypeID are complementary.  AddTypeID checks that the name is
		// not duplicate.  GetTypeID fails when the type is not found

		static TypeID AddTypeID(const char* typeName);
		static TypeID GetTypeID(const char* typeName);
	private:

		TypeID AddTypeIDInst(const char* typeName);
		TypeID GetTypeIDInst(const char* typeName) const;
		
		static TypeManager& GetInstance();

		mutable std::shared_mutex m_mutex;
		TypeID m_freeId{ TYPEID_FIRST_TYPED };
		std::unordered_map<std::string, TypeID> m_typeIds;
	};

	template<typename T>
	struct type_name
	{
		// If the type name is nullptr, then the type does not exist
		static constexpr char* value = nullptr;
	};

	template<typename T>
	constexpr char* type_name_v = type_name<T>::value;

	// Note: This object should be explicitly instantiated for described types
	// Otherwise there will be utter chaos.

	template<typename T>
	struct TypeInfo
	{
		static constexpr TypeID type_id = 
			type_name_v<T> ? TypeManager::AddTypeID(type_name_v<T>) : TYPEID_UNTYPED;
	};

}