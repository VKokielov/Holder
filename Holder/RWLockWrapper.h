#pragma once

#include <shared_mutex>

namespace holder::lib
{

	// The RWLockWrapper allows selective locking of shared locks based on thread-local
	// variables to prevent recursion and invalid states

	enum LockState
	{
		Unlocked,
		ReadLocked,
		WriteLocked
	};

	class SharedRecursionException { };

	class RWLockWrapperRead
	{
	public:
		RWLockWrapperRead(const std::shared_mutex& mutex,
			LockState& curState)
			:m_lockState(curState),
			m_slock(mutex, std::defer_lock)
		{
			if (m_lockState == LockState::Unlocked)
			{
				m_lockState = LockState::ReadLocked;
				m_slock.lock();
			}
		}

		~RWLockWrapperRead()
		{
			m_lockState = LockState::Unlocked;
		}
	private:
		LockState& m_lockState;
		std::shared_lock<std::shared_mutex> m_slock;
	};

	class RWLockWrapperWrite
	{
	public:
		RWLockWrapperWrite(std::shared_mutex& mutex,
			LockState& curState)
			:m_lockState(curState),
			m_ulock(mutex, std::defer_lock)
		{
			if (m_lockState == LockState::Unlocked)
			{
				m_lockState = LockState::WriteLocked;
				m_ulock.lock();
			}
			else if (m_lockState == LockState::ReadLocked)
			{
				// Can't increase lock strength without releasing the lock
				throw SharedRecursionException();
			}
		}

		~RWLockWrapperWrite()
		{
			m_lockState = LockState::Unlocked;
		}


	private:
		LockState& m_lockState;
		std::unique_lock<std::shared_mutex> m_ulock;
	};

}