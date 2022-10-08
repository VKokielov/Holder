#pragma once

#include "TypeInterface.h"
#include "IDStore.h"
#include "NumberUtils.h"

#include <memory>
#include <optional>
#include <string>

namespace holder::base
{

	struct TypeDesc
	{
		std::string typeName;
		std::shared_ptr<IType>  typeDef;
	};

	class TypeManager
	{
	private:
		using TypeCounter = uint32_t;
		struct Type_
		{

		};


	public:

		static TypeManager& GetInstance();

		// Find or create a type with a given name
		TypeID FindType(const TypeDesc& desc);

		
	private:
		lib::VectorBasedStore<TypeCounter, Type_> m_typeStore;
	};
}