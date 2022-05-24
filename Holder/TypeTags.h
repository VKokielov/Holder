#pragma once

#include <shared_mutex>
#include <unordered_map>
#include <string>
#include <functional>

namespace holder::base::types
{
	// Type tags are used, mainly, for dispatch
	// They can be queried as singletons from a single manager

	class DuplicateTypeException { };

	class TypeNotFoundException { };

	class InvalidObjTypeException { };

	using TypeID = uint32_t;

	class TypeTag
	{
	public:
		TypeTag(const TypeTag& other) = default;
		TypeTag& operator=(const TypeTag& other) = default;

		bool operator==(const TypeTag& other) const
		{
			return m_typeID == other.m_typeID;
		}

		std::size_t GetHash() const
		{
			static std::hash<TypeID> memberHash;
			return memberHash(m_typeID);
		}

	private:
		TypeTag(TypeID typeID)
			:m_typeID(typeID)
		{ }

		TypeID m_typeID;
		friend class TypeTagManager;
	};

	class TypeTagManager
	{
	public:
		static TypeTag GetType(const char* typeName);
	private:
		TypeTag GetTypeInst(const char* typeName);
		
		static TypeTagManager& GetInstance();

		mutable std::shared_mutex m_mutex;
		TypeID m_freeId{ 1 };
		std::unordered_map<std::string, TypeTag> m_typeMap;
	};

}

namespace std
{

	template<>
	struct hash<holder::base::types::TypeTag>
	{
		std::size_t operator()(const holder::base::types::TypeTag& tag) const
		{
			return tag.GetHash();
		}
	};

}