#pragma once

#include <string>
#include <cinttypes>
#include <memory>
#include <limits>
#include <vector>

namespace holder::data
{

	enum class BaseDatumType
	{
		Element,
		Dictionary,
		List,
		Object
	};

	enum class ElementType
	{
		None,
		Boolean,
		Int8,
		UInt8,
		Int16,
		UInt16,
		Int32,
		UInt32,
		Int64,
		UInt64,
		Float,
		Double,
		String
	};

	class DataStateException { };

	class IDatum
	{
	public:
		virtual ~IDatum() = default;
		virtual BaseDatumType GetDatumType() const = 0;
		virtual bool IsImmutable() const = 0;
	};

	class IElementDatum : public IDatum
	{
	public:
		virtual ElementType GetElementType() const = 0;

		virtual bool Get(bool& rDatum) const = 0;
		virtual void Set(bool datum) = 0;

		virtual bool Get(int8_t& rDatum) const = 0;
		virtual void Set(int8_t datum) = 0;

		virtual bool Get(uint8_t& rDatum) const = 0;
		virtual void Set(uint8_t datum) = 0;

		virtual bool Get(int16_t& rDatum) const = 0;
		virtual void Set(int16_t datum) = 0;

		virtual bool Get(uint16_t& rDatum) const = 0;
		virtual void Set(uint16_t datum) = 0;

		virtual bool Get(int32_t& rDatum) const = 0;
		virtual void Set(int32_t datum) = 0;

		virtual bool Get(uint32_t& rDatum) const = 0;
		virtual void Set(uint32_t datum) = 0;

		virtual bool Get(int64_t& rDatum) const = 0;
		virtual void Set(int64_t datum) = 0;

		virtual bool Get(uint64_t& rDatum) const = 0;
		virtual void Set(uint64_t datum) = 0;

		virtual bool Get(float& rDatum) const = 0;
		virtual void Set(float datum) = 0;

		virtual bool Get(double& rDatum) const = 0;
		virtual void Set(double datum) = 0;

		virtual bool Get(std::string& rDatum) const = 0;
		virtual void Set(const char* datum) = 0;

		virtual void Clear() = 0;  // reset to none
	};

	class IDictCallback
	{
	public:
		virtual ~IDictCallback() = default;
		virtual bool OnEntry(const char* pKey, const std::shared_ptr<IDatum>& pChild) = 0;
	};

	class IDictDatum : public IDatum
	{
	public:
		virtual bool IsEmpty() const = 0;
		virtual bool HasEntry(const char* pKey) const = 0;
		virtual bool GetEntry(const char* pKey, std::shared_ptr<IDatum>& rChild) const = 0;
		virtual bool Iterate(IDictCallback& rCallback) const = 0;

		virtual bool SetEntry(const char* pKey, const std::shared_ptr<IDatum>& rChild) = 0;
	};

	constexpr size_t LIST_NPOS = std::numeric_limits<size_t>::max();

	class IListDatum : public IDatum
	{
	public:
		virtual bool IsEmpty() const = 0;
		virtual size_t GetLength() const = 0;
		virtual bool GetEntry(size_t idx, std::shared_ptr<IDatum>& rChild) const = 0;
		// Iteration would not be faster than calling GetEntry() a number of times, but 
		// GetRange may be, although the interface must be hybridized
		virtual bool GetRange(std::vector<std::shared_ptr<IDatum> >& rSequence,
							  size_t startIdx = 0, 
							  size_t endIdx = LIST_NPOS) const = 0;

		virtual bool InsertEntry(const std::shared_ptr<IDatum>& pEntry,
			size_t indexBefore = LIST_NPOS) = 0;
		virtual bool SetEntry(const std::shared_ptr<IDatum>& pEntry,
			size_t indexAt) = 0;
	};

	class IObjectDatum : public IDatum
	{
	public:
		virtual const char* GetObjectType() const = 0;
		virtual IObjectDatum* MakeView() const = 0;
		virtual IObjectDatum* Clone() const = 0;
		virtual bool GetRepresentation(std::string& rRepr) const = 0;
	};

	class IDatumFactories
	{
	public:
		virtual IDatum* CreateElement() = 0;
		virtual IDatum* CreateDictionary() = 0;
		virtual IDatum* CreateList() = 0;
	};

	enum class IterInstruction
	{
		// Enter a compound datum
		// For simple data == skip
		Enter,
		// Skip a compound datum
		Skip,
		// Stop iterating
		Break
	};

	class ITreeSerializer
	{
	public:
		virtual ~ITreeSerializer() = default;

		virtual IterInstruction OnList(const std::shared_ptr<IListDatum>& pList,
			bool hasElements) = 0;
		virtual IterInstruction OnDict(const std::shared_ptr<IDictDatum>& pDict,
			bool hasElements) = 0;
		virtual IterInstruction OnDictKey(const char* pKey) = 0;
		virtual void EndCompound() = 0;

		virtual IterInstruction OnElement(const std::shared_ptr<IElementDatum>& pElement) = 0;
		virtual IterInstruction OnObject(const std::shared_ptr<IObjectDatum>& pObject) = 0;
	};

}