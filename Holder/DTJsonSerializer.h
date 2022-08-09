#pragma once

#include "IDataTree.h"
#include <string>

namespace holder::data
{
	enum class TokenType
	{
		Value,
		DictOpener,
		ListOpener,
		Closer,
		ListSeparator,
		DictSeparator // separates keys from values in a dictionary
	};

	struct JsonToken
	{
		TokenType tokenType;
		std::string sToken;
		// If this token starts a list or dictionary, represents whether
		// it has compound children
		bool hasCompounds;  
		// If this is an opener, index of the corresponding closing token
		size_t idxComplement;
	};

	// WARNING:  This class uses stringstreams.  It is NOT designed for maximum speed/performance
	class DTJsonTokenGenerator : public ITreeSerializer
	{
	public:
		IterInstruction OnList(const std::shared_ptr<IListDatum>& pList,
		    bool hasElements) override;
		IterInstruction OnDict(const std::shared_ptr<IDictDatum>& pDict,
		    bool hasElements) override;
		IterInstruction OnDictKey(const char* pKey) override;
		void EndCompound() override;

		IterInstruction OnElement(const std::shared_ptr<IElementDatum>& pElement) override;
		IterInstruction OnObject(const std::shared_ptr<IObjectDatum>& pObject) override;

		const std::vector<JsonToken>&
			GetTokenVector() const
		{
			return m_tokenList;
		}
	private:
		static std::string GetRepresentation(const std::shared_ptr<IElementDatum>& pElement);

		template<typename T>
		static std::string ToString(const std::shared_ptr<IElementDatum>& pDatum);
		
		// Call PushLevel _after_ creating a token
		void PushLevel();
		void PopLevel();
		void AddSeparator();

		// Token sequence
		std::vector<JsonToken>  m_tokenList;
		// Stack of indices for compounds
		std::vector<size_t> m_openerStack;
		// First element of a JSON, list or dictionary
		bool m_firstElement{ true };
	};

	// Format as an indented JSON
	std::string PrintJsonIndented(const std::vector<JsonToken>& tokens,
		unsigned int levelIndent,
		unsigned int sepSpaces,
		bool sameLineElements);

}