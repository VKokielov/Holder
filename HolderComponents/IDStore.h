#pragma once

#include <unordered_map>
#include <vector>
#include <cinttypes>
#include <optional>

namespace holder::lib
{
	/*
		Vector based store with unique IDs
		The ID consists of two numbers, an object ID and a generation
		The counter is a single value expressing the location of the object in the vector
		
		The generation is a value representing the number of objects previously removed
		in the slot.
	*/

	class IDStoreException { };

	template<typename TCounter>
	struct StoreID
	{
		TCounter idx;
		TCounter gen;
	};

	template<typename TObj, 
		typename TCounter = uint32_t>
	class VectorBasedStore
	{
	private:
		struct Slot
		{
			TCounter gen{ 0 };
			std::optional<TObj>  obj;
		};

	public:
		template<typename ... Args>
		TObj& Emplace(StoreID<TCounter>& outID, Args&& ... args)
		{
			TCounter destID{};

			if (m_freeStack.empty())
			{
				auto nextID = m_freeID;
				++nextID;

				if (nextID < m_freeID)
				{
					// Type overflow
					throw IDStoreException();
				}

				m_objs.emplace_back();
				destID = m_freeID;
				m_freeID = nextID;
			}
			else
			{
				destID = m_freeStack.back();
				m_freeStack.pop_back();
			}

			// As the ID is generated, there is no need to check whether the 
			// slot is free.  It is guaranteed as long as the object is intact

			m_objs[destID].obj.emplace(std::forward<Args>(args)...);

			// Generation was incremented during remove
			outID.gen = m_objs[destID].gen;
			outID.idx = destID;

			return m_objs[destID].obj;
		}

		TObj* Get(StoreID<TCounter> id)
		{
			return GetStatic<TObj*>(this, id);
		}
		const TObj* Get(StoreID<TCounter> id) const
		{
			return GetStatic<const TObj*>(this, id);
		}
		bool Remove(StoreID<TCounter> id)
		{
			if (id.idx < m_objs.size()
				&& m_objs[id.idx].gen == id.gen
				&& m_objs[id.idx].obj.has_value())
			{
#ifdef _DEBUG
				if (!m_objs[id.idx].obj.has_value())
				{
					throw IDStoreException();
				}
#endif

				// This ensures no existing ID properly obtained from this object will match
				++m_objs[idx].gen;
				m_objs[id.idx].reset();
				m_freeStack.push_back(id.idx);
				return true;
			}

			return false;
		}

	private:
		// const-agnostic function
		template<typename RetType, typename ThisType>
		static RetType GetStatic(ThisType* pThis,
			StoreID<TCounter> id)
		{
			RetType ret{ nullptr };

			if (id.idx < m_objs.size()
				&& m_objs[id.idx].gen == id.gen
				&& m_objs[id.idx].obj.has_value())
			{
				ret = &m_objs[id.idx].obj.value();
			}

			return ret;
		}

		std::vector<Slot> m_objs;
		std::vector<TCounter> m_freeStack;
		TCounter m_freeID{};
	};


}