#pragma once

#include "IAppObject.h"
#include "ISharedObject.h"
#include "RaiiLambda.h"
#include "TypeInfoObjs.h"
#include "MiniSpinMutex.h"

#include <cinttypes>
#include <memory>
#include <vector>
#include <shared_mutex>
#include <optional>
#include <unordered_set>
#include <atomic>

namespace holder::base
{
	using SharedObjectID = uint64_t;
	constexpr SharedObjectID INVALID_SHARED_OBJ = ~(0LL);

	enum class LockFreeState
	{
		None,
		SingleThread,
		Concurrent
	};

	enum class LockState
	{
		Unlocked,
		ReadLocked,
		WriteLocked
	};

	class SharedObjectStore;

	class LockStateException { };

	class SharedObjectWrapper
	{
	private:
		struct LockInfo_
		{
			LockState state{ LockState::Unlocked };
			unsigned int count{ 0 };
		};

	public:
		
		// Cooperates with the caller by giving out a concurrency that must be
		// returned to the wrapper
		LockFreeState SharedReadLock()
		{
			return SharedLockImpl(false);
		}

		LockFreeState SharedWriteLock()
		{
			return SharedLockImpl(true);
		}

		void SharedUnlock(LockFreeState lockState)
		{
			SharedUnlockImpl(lockState);
		}

		ISharedObject* Get()
		{
			return m_pObj.get();
		}

		TypeID GetTypeID() const
		{
			return m_typeId;
		}

	protected:
		SharedObjectWrapper(std::shared_ptr<ISharedObject> pObj,
			TypeID typeId)
			:m_pObj(pObj),
			m_objId(m_freeObjId++),
			m_typeId(typeId)
		{

		}

	private:
		LockFreeState SharedLockImpl(bool write);
		void SharedUnlockImpl(LockFreeState stateOnLock);
		void SharedReadLockSet();
		void SharedWriteLockSet();
		void SharedUnlockSet();

		static thread_local std::vector<LockInfo_>  m_lockState;
		static std::atomic<SharedObjectID>  m_freeObjId;

		SharedObjectID m_objId;

		lib::MiniSpinMutex m_lfStateMutex;

		LockFreeState   m_lfState{ LockFreeState::None };
		std::shared_mutex m_rwMutex;
		std::shared_ptr<ISharedObject> m_pObj;

		TypeID m_typeId;
	};

	template<typename T>
	class TypedSharedObjectWrapper : public SharedObjectWrapper
	{
	private:
		static_assert(std::is_base_of_v<ISharedObject, T>, "TypedSharedObjectWrapper: must refer to a subtype of ISharedObject");
	public:
		TypedSharedObjectWrapper(std::shared_ptr<T> pObj)
			:SharedObjectWrapper(pObj, TypeInfo<T>::type_id)
		{ }
	};

	// A class that can only be copied or moved by its owner (SharedObjectWrapper)
	// This does not prevent inopportune locking (for example outside a function)
	// but does help to prevent the escape of these objects
	// If they must be used by some function, they should be passed in as references
	// Better yet, use Get() and the underlying pointer
	template<typename T, bool checked = false>
	class LockedSharedObjectRef
	{
	private:
		static constexpr bool m_readLock = std::is_const_v<T>;
		static_assert(std::is_base_of_v<ISharedObject, T>, "LockedSharedObjectRef: must refer to a subtype of ISharedObject");
	public:
		LockedSharedObjectRef(const LockedSharedObjectRef&) = delete;
		LockedSharedObjectRef(LockedSharedObjectRef&&) = delete;

		LockedSharedObjectRef(SharedObjectWrapper& rWrapper)
			:m_owner(rWrapper)
		{
			if constexpr (checked)
			{
				if (m_owner.GetTypeID() != TypeInfo<T>::type_id)
				{
					throw InvalidObjTypeException();
				}
			}

			m_ptr = static_cast<T*>(m_owner.Get());

			// Lock
			if constexpr (m_readLock)
			{
				m_concurrencyToken = m_owner.SharedReadLock();
			}
			else
			{
				m_concurrencyToken = m_owner.SharedWriteLock();
			}
		}

		~LockedSharedObjectRef()
		{
			m_owner.SharedUnlock(m_concurrencyToken);
		}

		T* Get()
		{
			return m_ptr;
		}

		T& operator*()
		{
			return *m_ptr;
		}

		T* operator->()
		{
			return m_ptr;
		}

	private:
		SharedObjectWrapper&  m_owner;
		LockFreeState m_concurrencyToken;
		T* m_ptr;
	};

	using SharedObjectPtr = std::shared_ptr<SharedObjectWrapper>;

	template<typename T,
		typename Fac,
		typename Initializer,
		typename ... Args>
		std::shared_ptr<SharedObjectWrapper>
		MakeInitializedSharedObjectWithFactory(Fac&& fac,
			Initializer&& init,
			Args&& ... args)
	{
		static_assert(std::is_base_of_v<ISharedObject, T>, "MakeSharedObject can only create subtypes of ISharedObject");
		
		std::shared_ptr<T> pObj{ fac(std::forward<Args>(args)...) };
		init(pObj);

		auto pWrapper = std::make_shared<TypedSharedObjectWrapper<T> >(pObj);
		return pWrapper;
	}

	template<typename T,
		typename Fac,
		typename ... Args>
		std::shared_ptr<SharedObjectWrapper>
		MakeSharedObjectWithFactory(Fac&& fac,
			Args&& ... args)
	{
		static auto blankInit = [](auto& pobj) {};

		return MakeInitializedSharedObjectWithFactory<T>(std::forward<Fac>(fac),
			blankInit,
			std::forward<Args>(args)...);
	}

	template<typename T, typename ... Args>
	std::shared_ptr<SharedObjectWrapper>
		MakeStandardSharedObject(Args&& ...args)
	{
		auto objFac = [](Args&& ... lambda_args)
		{
			return new T(std::forward<Args>(lambda_args)...);
		};

		return MakeSharedObjectWithFactory<T>(objFac, std::forward<Args>(args)...);
	}

}