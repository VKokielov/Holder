#pragma once

#include <atomic>
#include <mutex>

namespace holder::lib
{

	class MiniSpinMutex
	{
	public:

		void lock()
		{
			bool expVal{ false };
			while (!m_taken.compare_exchange_strong(expVal, true));
		}

		void unlock()
		{
			m_taken.store(false);
		}
	private:
		std::atomic<bool>  m_taken{ false };
	};

}