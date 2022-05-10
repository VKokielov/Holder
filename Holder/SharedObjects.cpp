#include "SharedObjects.h"
#include "RaiiLambda.h"

namespace impl_ns = holder::base;

std::atomic<impl_ns::SharedObjectID>  impl_ns::SharedObjectWrapper::m_freeObjId{ 0 };
thread_local std::vector<impl_ns::SharedObjectWrapper::LockInfo_> impl_ns::SharedObjectWrapper::m_lockState;

impl_ns::LockFreeState impl_ns::SharedObjectWrapper::SharedLockImpl(bool write)
{
	std::unique_lock lkSpin(m_lfStateMutex);

	// Everyone starts out lock-free.  
	if (m_lfState == LockFreeState::None)
	{
		m_lfState = LockFreeState::SingleThread;
		return LockFreeState::SingleThread;
	}

	// If we're in single-thread state, we give the other thread
	// a chance to finish and then take action by upgrading to 
	// concurrent
	// This could be an inefficient wait, but it should only happen
	// when a previously single-thread object is accessed from a second thread
	if (m_lfState == LockFreeState::SingleThread)
	{
		while (m_lfState != LockFreeState::None)
		{
			lkSpin.unlock();
			lkSpin.lock();
		}

		// Upgrade to concurrent
		// NOTE:  Some other thread may have set this flag but we don't care;
		// it is the final state
		m_lfState = LockFreeState::Concurrent;
	}

	// At this point we are certainly concurrent
	if (write)
	{
		SharedWriteLockSet();
	}
	else
	{
		SharedReadLockSet();
	}

	return LockFreeState::Concurrent;
}

void impl_ns::SharedObjectWrapper::SharedUnlockImpl(LockFreeState stateOnLock)
{
	std::unique_lock lkSpin(m_lfStateMutex);

	if (stateOnLock == LockFreeState::None)
	{
		throw LockStateException();
	}

	if (stateOnLock == LockFreeState::Concurrent)
	{
		SharedUnlockSet();
	}
	else // single-threaded
	{
		m_lfState = LockFreeState::None;
	}
}

void impl_ns::SharedObjectWrapper::SharedReadLockSet()
{
	if (m_objId >= m_lockState.size())
	{
		m_lockState.resize(m_objId + 1);
	}

	if (m_lockState[m_objId].state == LockState::WriteLocked)
	{
		throw LockStateException();
	}
	else if (m_lockState[m_objId].state == LockState::Unlocked)
	{
		m_rwMutex.lock();
		m_lockState[m_objId].state = LockState::ReadLocked;
	}

	++m_lockState[m_objId].count;
}

void impl_ns::SharedObjectWrapper::SharedWriteLockSet()
{
	if (m_objId >= m_lockState.size())
	{
		m_lockState.resize(m_objId + 1);
	}

	if (m_lockState[m_objId].state == LockState::Unlocked)
	{
		m_rwMutex.lock();
		m_lockState[m_objId].state = LockState::WriteLocked;
	}

	++m_lockState[m_objId].count;
}

void impl_ns::SharedObjectWrapper::SharedUnlockSet()
{
	if (m_objId >= m_lockState.size()
		|| m_lockState[m_objId].state == LockState::Unlocked)
	{
		throw LockStateException();
	}

	if (m_lockState[m_objId].count == 1)
	{
		if (m_lockState[m_objId].state == LockState::ReadLocked)
		{
			m_rwMutex.unlock_shared();
		}
		else
		{
			m_rwMutex.unlock();
		}

		m_lockState[m_objId].state = LockState::Unlocked;

	}

	--m_lockState[m_objId].count;
}