#pragma once
#include "IDataTree.h"
#include "DTConstruct.h"
#include <type_traits>
#include <limits>

namespace holder::data
{

	bool SerializeDataTree(const std::shared_ptr<IDatum>& pRoot,
		ITreeSerializer& serializer);

	enum class AccessResult
	{
		OK,
		Narrowed, // was able to get the value, but narrowed
		NotDictionary,
		NotList,
		NoSuchElement,
		WrongTargetType,
		WrongElementType
	};

	template<typename T>
	struct DatumTraits;

	template<>
	struct DatumTraits<IElementDatum>
	{
		static constexpr BaseDatumType tag = BaseDatumType::Element;
	};

	template<>
	struct DatumTraits<IObjectDatum>
	{
		static constexpr BaseDatumType tag = BaseDatumType::Object;
	};

	template<>
	struct DatumTraits<IListDatum>
	{
		static constexpr BaseDatumType tag = BaseDatumType::List;
	};

	template<>
	struct DatumTraits<IDictDatum>
	{
		static constexpr BaseDatumType tag = BaseDatumType::Dictionary;
	};

	template<typename B>
	struct ElementTraits;

	template<>
	struct ElementTraits<bool>
	{
		static constexpr ElementType tag = ElementType::Boolean;
	};
	template<>
	struct ElementTraits<int8_t>
	{
		static constexpr ElementType tag = ElementType::Int8;
	};
	template<>
	struct ElementTraits<uint8_t>
	{
		static constexpr ElementType tag = ElementType::UInt8;
	};
	template<>
	struct ElementTraits<int16_t>
	{
		static constexpr ElementType tag = ElementType::Int16;
	};
	template<>
	struct ElementTraits<uint16_t>
	{
		static constexpr ElementType tag = ElementType::UInt16;
	};
	template<>
	struct ElementTraits<int32_t>
	{
		static constexpr ElementType tag = ElementType::Int32;
	};
	template<>
	struct ElementTraits<uint32_t>
	{
		static constexpr ElementType tag = ElementType::UInt32;
	};
	template<>
	struct ElementTraits<int64_t>
	{
		static constexpr ElementType tag = ElementType::Int64;
	};
	template<>
	struct ElementTraits<uint64_t>
	{
		static constexpr ElementType tag = ElementType::UInt64;
	};
	template<>
	struct ElementTraits<float>
	{
		static constexpr ElementType tag = ElementType::Float;
	};
	template<>
	struct ElementTraits<double>
	{
		static constexpr ElementType tag = ElementType::Double;
	};
	template<>
	struct ElementTraits<std::string>
	{
		static constexpr ElementType tag = ElementType::String;
	};

	enum class ConvertResult
	{
		OK,
		Narrowing,
		NoConversion
	};

	template<typename From, typename To>
	ConvertResult ConvertValue(To& destValue, const From& sourceValue)
	{
		if constexpr (std::is_integral_v<To> && std::is_integral_v<From>)
		{
			// Convert integral to integral
			// Decide whether the conversion is narrowing
			constexpr bool unsignedFrom =
				std::is_unsigned_v<From>;
			constexpr bool unsignedTo =
				std::is_unsigned_v<To>;

			// Safe to run this in the compiler for two signed or unsigned types
			constexpr bool widerToMax
				= (uint64_t)(std::numeric_limits<To>::max()) > (uint64_t)(std::numeric_limits<From>::max());

			bool isNarrowing{ false };

			if constexpr (unsignedFrom == unsignedTo)
			{
				constexpr bool widerToMin
					= unsignedFrom // this means that if both types are unsigned then the next condition won't be evaluated 
				   || (uint64_t)(-std::numeric_limits<To>::min()) > (uint64_t)(-std::numeric_limits<From>::min());

				if constexpr (!widerToMin
					|| !widerToMax)
				{
					if (sourceValue < (From)std::numeric_limits<To>::min())
					{
						destValue = std::numeric_limits<To>::min();
						isNarrowing = true;
					}
					else if (sourceValue > (From)std::numeric_limits<To>::max())
					{
						destValue = std::numeric_limits<To>::max();
						isNarrowing = true;
					}
					else
					{
						destValue = (To)sourceValue;
					}
				}
				else  // Safe (always widening) conversion
				{
					destValue = (To)sourceValue;
				}
			}
			else if constexpr (unsignedFrom)
			{
				if constexpr (!widerToMax)
				{
					if (sourceValue > (From)std::numeric_limits<To>::max())
					{
						destValue = std::numeric_limits<To>::max();
						isNarrowing = true;
					}
					else
					{
						destValue = (To)sourceValue;
					}
				}
				else
				{
					destValue = (To)sourceValue;
				}
				
			}
			else if constexpr (unsignedTo)
			{
				if (sourceValue < 0)
				{
					destValue = 0;
					isNarrowing = true;
				}
				else
				{
					if constexpr (!widerToMax)
					{
						if (sourceValue > (From)std::numeric_limits<To>::max())
						{
							destValue = std::numeric_limits<To>::max();
							isNarrowing = true;
						}
						else
						{
							destValue = (To)sourceValue;
						}
					}
					else
					{
						destValue = (To)sourceValue;
					}
				}
			}

			return isNarrowing ? ConvertResult::Narrowing : ConvertResult::OK;
		}
		else if constexpr (std::is_integral_v<To> && std::is_floating_point_v<From>)
		{
			// A narrowing conversion from float to integral
			destValue = (To)sourceValue;
			return ConvertResult::Narrowing;
		}
		else if constexpr (std::is_floating_point_v<From> && std::is_integral_v<To>)
		{
			destValue = (To)sourceValue;
			return ConvertResult::OK;
		}
		else if constexpr (std::is_same_v<From, double> && std::is_same_v<To, float>)
		{
			destValue = (To)sourceValue;
			return ConvertResult::Narrowing;
		}
		else if constexpr (std::is_same_v<From, float> && std::is_same_v<To, double>)
		{
			destValue = (To)sourceValue;
			return ConvertResult::OK;
		}
		else
		{
			return ConvertResult::NoConversion;
		}
	}

	/*
	template<typename From, typename To, 
			std::enable_if_t<std::is_integral_v<From> && std::is_integral_v<To>,bool> = true>
		ConvertResult ConvertValue(To& destValue, const From& sourceValue)
	{

	}
	*/

	// Accessors for closures
	template<typename T>
	AccessResult GetDictChild(const IDatum& datum,
		const char* pKey,
		std::shared_ptr<T>& rChild)
	{
		static_assert(std::is_base_of_v<IDatum, T>, "GetDictChild: T must derive from IDatum");

		if (datum.GetDatumType() != BaseDatumType::Dictionary)
		{
			return AccessResult::NotDictionary;
		}

		const IDictDatum& dictDatum = static_cast<const IDictDatum&>(datum);

		// Two cases: base class or derived class with type check
		if constexpr (std::is_same_v<IDatum, T>)
		{
			if (!dictDatum.GetEntry(pKey, rChild))
			{
				return AccessResult::NoSuchElement;
			}
		}
		else
		{
			std::shared_ptr<IDatum> genericChild;
			if (!dictDatum.GetEntry(pKey, genericChild))
			{
				return AccessResult::NoSuchElement;
			}

			if (genericChild->GetDatumType() != DatumTraits<T>::tag)
			{
				return AccessResult::WrongTargetType;
			}

			rChild = std::static_pointer_cast<T>(std::move(genericChild));
		}
		return AccessResult::OK;
	}

	template<typename T>
	AccessResult GetListChild(const IDatum& datum,
		size_t idx,
		std::shared_ptr<T>& rChild)
	{
		static_assert(std::is_base_of_v<IDatum, T>, "GetListChild: T must derive from IDatum");

		if (datum.GetDatumType() != BaseDatumType::List)
		{
			return AccessResult::NotList;
		}

		const IListDatum& listDatum = static_cast<const IListDatum&>(datum);
		
		if constexpr (std::is_same_v<IDatum, T>)
		{
			if (!listDatum.GetEntry(idx, rChild))
			{
				return AccessResult::NoSuchElement;
			}
		}
		else
		{
			std::shared_ptr<IDatum> genericChild;
			if (!listDatum.GetEntry(idx, genericChild))
			{
				return AccessResult::NoSuchElement;
			}

			if (genericChild->GetDatumType() != DatumTraits<T>::tag)
			{
				return AccessResult::WrongTargetType;
			}

			rChild = std::static_pointer_cast<T>(std::move(genericChild));
		}
		return AccessResult::OK;
	}

	// Accessor for element values
	template<typename From, typename To>
	AccessResult GetValueCoercedFrom(const IElementDatum& datum,
		To& destValue)
	{
		From sourceValue;

		if (!datum.Get(sourceValue))
		{
			return AccessResult::WrongElementType;
		}

		ConvertResult convResult = ConvertValue(destValue, sourceValue);

		if (convResult == ConvertResult::NoConversion)
		{
			return AccessResult::WrongElementType;
		}
		else if (convResult == ConvertResult::Narrowing)
		{
			return AccessResult::Narrowed;
		}

		return AccessResult::OK;
	}

	template<typename B>
	AccessResult GetCoercedValue(const IElementDatum& datum,
		ElementType elType,
		B& value)
	{
		switch (elType)
		{
		case ElementType::Int8:
			return GetValueCoercedFrom<int8_t, B>(datum, value);
		case ElementType::UInt8:
			return GetValueCoercedFrom<uint8_t, B>(datum, value);
		case ElementType::Int16:
			return GetValueCoercedFrom<int16_t, B>(datum, value);
		case ElementType::UInt16:
			return GetValueCoercedFrom<uint16_t, B>(datum, value);
		case ElementType::Int32:
			return GetValueCoercedFrom<int32_t, B>(datum, value);
		case ElementType::UInt32:
			return GetValueCoercedFrom<uint32_t, B>(datum, value);
		case ElementType::Int64:
			return GetValueCoercedFrom<int64_t, B>(datum, value);
		case ElementType::UInt64:
			return GetValueCoercedFrom<uint64_t, B>(datum, value);
		case ElementType::Float:
			return GetValueCoercedFrom<float, B>(datum, value);
		case ElementType::Double:
			return GetValueCoercedFrom<double, B>(datum, value);
		// No coercions from (or to) strings, or booleans
		case ElementType::Boolean:
		case ElementType::String:
		case ElementType::None:
			return AccessResult::WrongElementType;
		}

		return AccessResult::WrongElementType;
	}

	template<typename B>
	AccessResult GetValue(const IElementDatum& datum,
		B& value)
	{
		auto elType = datum.GetElementType();
		if (elType != ElementTraits<B>::tag)
		{
			// Coerce the value with a conversion
			return GetCoercedValue<B>(datum, elType, value);
		}

		datum.Get(value);
		return AccessResult::OK;
	}
		  
	// Accessors for values inside closures
	template<typename B>
	AccessResult GetDictValue(const IDatum& datum,
		const char* pKey,
		B& value)
	{
		std::shared_ptr<IElementDatum> pEl;
		auto childResult = GetDictChild<IElementDatum>(datum, pKey, pEl);
		if (childResult != AccessResult::OK)
		{
			return childResult;
		}

		auto valResult = GetValue(*pEl, value);
		return valResult;
	}

	template<typename B>
	AccessResult GetListValue(const IDatum& datum,
		size_t idx,
		B& value)
	{
		std::shared_ptr<IElementDatum> pEl;
		auto childResult = GetListChild<IElementDatum>(datum, idx, pEl);
		if (childResult != AccessResult::OK)
		{
			return childResult;
		}

		auto valResult = GetValue(pEl, value);
		return valResult;
	}

}