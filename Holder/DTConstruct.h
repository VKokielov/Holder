#pragma once
#include "IDataTree.h"
#include <type_traits>
#include <initializer_list>

namespace holder::data
{

	template<typename Suite, typename B>
	std::shared_ptr<IElementDatum> DTElem(B&& val)
	{
		using T = typename Suite::Element;

		static_assert(std::is_base_of_v<IElementDatum, T>, "DTElem: specify an implementation of IElementDatum");

		std::shared_ptr<IElementDatum> pDatum = std::make_shared<T>();
		pDatum->Set(std::forward<B>(val));

		return pDatum;
	}

	struct DictPair
	{
		std::string key;
		std::shared_ptr<IDatum> val;

		DictPair(const std::string& key_,
				 const std::shared_ptr<IDatum>& val_)
			:key(key_),
			val(val_)
		{ }
	};

	template<typename Suite>
	std::shared_ptr<IDictDatum> DTDict(std::initializer_list<DictPair> dictPairs)
	{
		using T = typename Suite::Dict;

		static_assert(std::is_base_of_v<IDictDatum, T>, "DTDict: specify an implementation of IDictDatum");
		std::shared_ptr<IDictDatum> pDatum = std::make_shared<T>();

		for (const DictPair& dictPair : dictPairs)
		{
			pDatum->SetEntry(dictPair.key.c_str(), dictPair.val);
		}

		return pDatum;
	}

	template<typename Suite>
	std::shared_ptr<IListDatum> DTList(std::initializer_list<std::shared_ptr<IDatum> > listEntries)
	{
		using T = typename Suite::List;

		static_assert(std::is_base_of_v<IListDatum, T>, "DTList: specify an implementation of IListDatum");
		std::shared_ptr<IListDatum> pDatum = std::make_shared<T>();

		for (const std::shared_ptr<IDatum>& listEntry : listEntries)
		{
			pDatum->InsertEntry(listEntry);
		}

		return pDatum;
	}


}