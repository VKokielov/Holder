#include "DTJsonSerializer.h"

#include <string_view>
#include <sstream>

namespace impl_ns = holder::data;

namespace
{
	constexpr std::string_view LIST_OPENER_TOKEN( "[" );
	constexpr std::string_view LIST_CLOSER_TOKEN( "]" );
	constexpr std::string_view DICT_OPENER_TOKEN("{");
	constexpr std::string_view DICT_CLOSER_TOKEN("}");
	constexpr std::string_view ELEMENT_SEPARATOR(",");
	constexpr std::string_view DICT_SEPARATOR(":");

	constexpr std::string_view NULL_REP{ "null" };
	constexpr std::string_view TRUE_REP{ "true" };
	constexpr std::string_view FALSE_REP{ "false" };

}

void impl_ns::DTJsonTokenGenerator::PushLevel()
{
	if (!m_openerStack.empty())
	{
		// We consider as a compound element any list or dictionary that is not empty
		JsonToken& openToken = m_tokenList[m_openerStack.back()];
		openToken.hasCompounds = true;
	}

	m_openerStack.emplace_back(m_tokenList.size() - 1);
	m_firstElement = true;
}
void impl_ns::DTJsonTokenGenerator::PopLevel()
{
	if (m_openerStack.empty())
	{
		throw DataStateException();
	}

	auto openerIdx = m_openerStack.back();
	JsonToken& openToken = m_tokenList[openerIdx];
	openToken.idxComplement = m_tokenList.size();

	std::string closerText;
	if (openToken.tokenType == TokenType::ListOpener)
	{
		closerText = LIST_CLOSER_TOKEN;
	}
	else if (openToken.tokenType == TokenType::DictOpener)
	{
		closerText = DICT_CLOSER_TOKEN;
	}
	else
	{
		throw DataStateException();
	}

	JsonToken closeToken{ TokenType::Closer, closerText, false, openerIdx };
	m_tokenList.emplace_back(std::move(closeToken));

	m_openerStack.pop_back();
	m_firstElement = false;
}

void impl_ns::DTJsonTokenGenerator::AddSeparator()
{

	JsonToken separatorToken{ TokenType::ListSeparator,
							 std::string(ELEMENT_SEPARATOR),
							 false,
							 0 };

	m_tokenList.emplace_back(separatorToken);
}

impl_ns::IterInstruction impl_ns::DTJsonTokenGenerator::OnList(const std::shared_ptr<IListDatum>& pList,
	bool hasElements)
{
	// Separator?
	if (m_firstElement)
	{
		m_firstElement = false;
	}
	else
	{
		AddSeparator();
	}

	// Create a list token and push or close
	JsonToken openToken{ TokenType::ListOpener, std::string(LIST_OPENER_TOKEN), false, 0 };
	m_tokenList.emplace_back(std::move(openToken));
	if (hasElements)
	{
		PushLevel();
	}
	else
	{
		// Set the complement index to the next token in the list
		m_tokenList.back().idxComplement = m_tokenList.size();
		JsonToken closeToken{ TokenType::Closer, std::string(LIST_CLOSER_TOKEN), false, m_tokenList.size() - 1};
		m_tokenList.emplace_back(std::move(closeToken));
	}

	return IterInstruction::Enter;
}
impl_ns::IterInstruction impl_ns::DTJsonTokenGenerator::OnDict(const std::shared_ptr<IDictDatum>& pDict,
	bool hasElements)
{
	if (m_firstElement)
	{
		m_firstElement = false;
	}
	else
	{
		AddSeparator();
	}

	// Create a dict token and push or close
	JsonToken openToken{ TokenType::DictOpener, std::string(DICT_OPENER_TOKEN), false, 0 };
	m_tokenList.emplace_back(std::move(openToken));
	if (hasElements)
	{
		PushLevel();
	}
	else
	{
		// Set the complement index to the next token in the list
		m_tokenList.back().idxComplement = m_tokenList.size();
		JsonToken closeToken{ TokenType::Closer, std::string(DICT_CLOSER_TOKEN), false, m_tokenList.size() - 1 };
		m_tokenList.emplace_back(std::move(closeToken));
	}

	return IterInstruction::Enter;
}

impl_ns::IterInstruction impl_ns::DTJsonTokenGenerator::OnDictKey(const char* pKey)
{
	if (m_firstElement)
	{
		m_firstElement = false;
	}
	else
	{
		AddSeparator();
	}

	std::stringstream ssm;
	ssm << "\"" << pKey << "\"";
	JsonToken keyToken{ TokenType::Value, ssm.str(), false, 0 };
	m_tokenList.emplace_back(std::move(keyToken));
	JsonToken sepToken{ TokenType::DictSeparator, std::string(DICT_SEPARATOR), false, 0 };
	m_tokenList.emplace_back(std::move(sepToken));

	// Special case -- no comma expected before the representation of the 
	// next element
	m_firstElement = true;

	return IterInstruction::Enter;
}

void impl_ns::DTJsonTokenGenerator::EndCompound()
{
	PopLevel();
}

impl_ns::IterInstruction impl_ns::DTJsonTokenGenerator::OnElement(const std::shared_ptr<IElementDatum>& pElement)
{
	if (m_firstElement)
	{
		m_firstElement = false;
	}
	else
	{
		AddSeparator();
	}
	
	std::string elementRep = GetRepresentation(pElement);
	JsonToken elementToken{ TokenType::Value, elementRep, false, 0 };
	m_tokenList.emplace_back(elementToken);
	return IterInstruction::Enter;
}
impl_ns::IterInstruction impl_ns::DTJsonTokenGenerator::OnObject(const std::shared_ptr<IObjectDatum>& pObject)
{
	if (m_firstElement)
	{
		m_firstElement = false;
	}
	else
	{
		AddSeparator();
	}

	// Add the object rep
	std::string objRep;
	if (!pObject->GetRepresentation(objRep))
	{
		objRep = "<repr-error>";
	}

	std::stringstream ssm;
	ssm << "\"" << objRep << "\"";
	
	JsonToken objToken{ TokenType::Value, ssm.str(), false, 0 };
	m_tokenList.emplace_back(objToken);
	return IterInstruction::Enter;
}

template<typename T>
std::string impl_ns::DTJsonTokenGenerator::ToString(const std::shared_ptr<IElementDatum>& pDatum)
{
	T dataVal;
	if (!pDatum->Get(dataVal))
	{
		throw DataStateException();
	}

	std::stringstream ssm;
	ssm << dataVal;
	return ssm.str();
}

std::string impl_ns::DTJsonTokenGenerator::GetRepresentation(const std::shared_ptr<IElementDatum>& pElement)
{
	switch (pElement->GetElementType())
	{
		case ElementType::None:
			return std::string(NULL_REP);
		case ElementType::Boolean:
		{
			bool bval{ false };
			if (!pElement->Get(bval))
			{
				// error...
				break;
			}
			return std::string(bval ? TRUE_REP : FALSE_REP);
		}
		case ElementType::Int8:
			return ToString<int8_t>(pElement);
		case ElementType::UInt8:
			return ToString<uint8_t>(pElement);
		case ElementType::Int16:
			return ToString<int16_t>(pElement);
		case ElementType::UInt16:
			return ToString<uint16_t>(pElement);
		case ElementType::Int32:
			return ToString<int32_t>(pElement);
		case ElementType::UInt32:
			return ToString<uint32_t>(pElement);
		case ElementType::Int64:
			return ToString<int64_t>(pElement);
		case ElementType::UInt64:
			return ToString<uint64_t>(pElement);
		case ElementType::Float:
			return ToString<float>(pElement);
		case ElementType::Double:
			return ToString<double>(pElement);
		case ElementType::String:
		{
			std::string strRep;
			if (!pElement->Get(strRep))
			{
				break;
			}

			std::stringstream ssm;
			ssm << "\"" << strRep << "\"";
			return ssm.str();
		}
	}

	throw DataStateException();
}

std::string impl_ns::PrintJsonIndented(const std::vector<JsonToken>& tokens,
	unsigned int levelIndent,
	unsigned int sepSpaces,
	bool sameLineElements)
{
	unsigned int indentLevel{ 0 };
	std::stringstream ssmFormatted;
	std::vector<size_t> compoundStack;
	
	for (size_t i = 0; i < tokens.size(); ++i)
	{
		const JsonToken& curToken = tokens[i];
		ssmFormatted << curToken.sToken;

		if (curToken.tokenType == TokenType::DictOpener
			|| curToken.tokenType == TokenType::ListOpener)
		{
			compoundStack.push_back(i);
		}

		bool addSpaces{ false };
		bool addNewline{ false };
		int indentDelta{ 0 };

		if (curToken.tokenType == TokenType::DictSeparator)
		{
			addSpaces = true;
		}
		else if (curToken.tokenType == TokenType::DictOpener
			|| curToken.tokenType == TokenType::ListOpener)
		{
			// Add a newline and indentation iff this has compounds
			addNewline = !sameLineElements || curToken.hasCompounds;
			if (addNewline)
			{
				indentDelta = 1;
			}
		}
		else if (curToken.tokenType == TokenType::ListSeparator)
		{
			if (compoundStack.empty())
			{
				throw DataStateException();
			}

			addNewline = !sameLineElements || tokens[compoundStack.back()].hasCompounds;
			if (!addNewline)
			{
				addSpaces = true;
			}
		}
		else if (curToken.tokenType == TokenType::Closer)
		{
			if ((i < tokens.size() - 1)
			&& (tokens[i + 1].tokenType == TokenType::Closer))
			{
				if (compoundStack.size() < 2)
				{
					throw DataStateException();
				}

				// This should mean there are at least two elements in the token stack
				// We need to get the outer closer's behavior
				addNewline =
					  !sameLineElements
					|| (tokens[compoundStack[compoundStack.size() - 2]].hasCompounds);

				if (addNewline)
				{
					indentDelta = -1;
				}
			}
		}
		else if (!compoundStack.empty())
		{
			if (i == tokens.size() - 1)
			{
				throw DataStateException();
			}

			// Special case - add a newline for the last element of a list or dictionary
			addNewline = (tokens[i+1].tokenType == TokenType::Closer)
				&& (!sameLineElements || tokens[compoundStack.back()].hasCompounds);

			if (addNewline)
			{
				indentDelta = -1;
			}
		}

		indentLevel += indentDelta;

		if (addNewline)
		{
			ssmFormatted << '\n';
			for (unsigned int spc = 0; spc < levelIndent * indentLevel; ++spc)
			{
				ssmFormatted << ' ';
			}
		}
		else if (addSpaces)
		{
			for (unsigned int spc = 0; spc < sepSpaces; ++spc)
			{
				ssmFormatted << ' ';
			}
		}

		if (curToken.tokenType == TokenType::Closer)
		{
			if (compoundStack.empty())
			{
				throw DataStateException();
			}
			compoundStack.pop_back();
		}
	}

	return ssmFormatted.str();
}