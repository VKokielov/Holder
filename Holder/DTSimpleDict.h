#pragma once
#include "IDataTree.h"
#include <unordered_map>

namespace holder::data::simple
{

	class Dictionary : public IDictDatum
	{
	public:
		bool IsEmpty() const override;
		BaseDatumType GetDatumType() const override;
		bool IsImmutable() const override;

		bool HasEntry(const char* pKey) const override;
		bool GetEntry(const char* pKey, std::shared_ptr<IDatum>& rChild) const override;
		bool Iterate(IDictCallback& rCallback) const override;

		bool SetEntry(const char* pKey, const std::shared_ptr<IDatum>& rChild) override;

	private:
		std::unordered_map<std::string, std::shared_ptr<IDatum> >
			m_dictionary;
	};

}