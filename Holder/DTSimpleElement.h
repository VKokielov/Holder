#include "IDataTree.h"
#include <variant>

namespace holder::data::simple
{

	class Element : public IElementDatum
	{
	private:
		struct NoneType { };
		static constexpr std::size_t NoneIndex = 0;
		static constexpr std::size_t BoolIndex = 1;
		static constexpr std::size_t Int8Index = 2;
		static constexpr std::size_t UInt8Index = 3;
		static constexpr std::size_t Int16Index = 4;
		static constexpr std::size_t UInt16Index = 5;
		static constexpr std::size_t Int32Index = 6;
		static constexpr std::size_t UInt32Index = 7;
		static constexpr std::size_t Int64Index = 8;
		static constexpr std::size_t UInt64Index = 9;
		static constexpr std::size_t FloatIndex = 10;
		static constexpr std::size_t DoubleIndex = 11;
		static constexpr std::size_t StringIndex = 12;

		static ElementType IndexToType(std::size_t idx)
		{
			switch (idx)
			{
			case NoneIndex: return ElementType::None;
			case BoolIndex: return ElementType::Boolean;
			case Int8Index: return ElementType::Int8;
			case UInt8Index: return ElementType::UInt8;
			case Int16Index: return ElementType::Int16;
			case UInt16Index: return ElementType::UInt16;
			case Int32Index: return ElementType::Int32;
			case UInt32Index: return ElementType::UInt32;
			case Int64Index: return ElementType::Int64;
			case UInt64Index: return ElementType::UInt64;
			case FloatIndex: return ElementType::Float;
			case DoubleIndex: return ElementType::Double;
			case StringIndex: return ElementType::String;
			}

			return ElementType::None;
		}

	public:
		BaseDatumType GetDatumType() const override;
		bool IsImmutable() const override;
		ElementType GetElementType() const override;

		bool Get(bool& rDatum) const override;
		void Set(bool datum) override;

		bool Get(int8_t& rDatum) const override;
		void Set(int8_t datum) override;

		bool Get(uint8_t& rDatum) const override;
		void Set(uint8_t datum) override;

		bool Get(int16_t& rDatum) const override;
		void Set(int16_t datum) override;

		bool Get(uint16_t& rDatum) const override;
		void Set(uint16_t datum) override;

		bool Get(int32_t& rDatum) const override;
		void Set(int32_t datum) override;

		bool Get(uint32_t& rDatum) const override;
		void Set(uint32_t datum) override;

		bool Get(int64_t& rDatum) const override;
		void Set(int64_t datum) override;

		bool Get(uint64_t& rDatum) const override;
		void Set(uint64_t datum) override;

		bool Get(float& rDatum) const override;
		void Set(float datum) override;

		bool Get(double& rDatum) const override;
		void Set(double datum) override;

		bool Get(std::string& rDatum) const override;
		void Set(const char* datum) override;

		void Clear() override;
	private:
		template<typename T>
		bool GenericGet(T& dest) const;

		template<typename T>
		void GenericSet(T&& val);

		std::variant<NoneType,
			bool,
			int8_t,
			uint8_t,
			int16_t,
			uint16_t,
			int32_t,
			uint32_t,
			int64_t,
			uint64_t,
			float,
			double,
			std::string>
			m_data;
	};



}