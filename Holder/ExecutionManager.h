#pragma once
#include "IExecutor.h"

#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <string>
#include <condition_variable>
#include <shared_mutex>
#include <chrono>

namespace holder::base
{
	using ExecutorID = uint32_t;

	constexpr ExecutorID EXEC_WILDCARD = 0xffffffff;

	enum class ExecutorSignalType
	{
		WakeUp,
		RequestTermination
	};

	class ExecutorStateException { };

	class ExecutionManager
	{
	private:
		using ThreadID = size_t;
		using LocalExecutorID = size_t;

		enum class ExecutionInstructionTag
		{
			AddExecutor,
			SignalExecutor
		};

		struct ExecutionInstructionMessage
		{
			ExecutionInstructionTag instructionTag;
			ExecutorSignalType signalType;
			ExecutorID execId;
			std::shared_ptr<IExecutor> executor;
		};

		class ExecutorThread 
		{
		private:
			enum class ExecutorStateTag
			{
				Executing,
				Suspended
			};

			struct ExecutorState
			{
				ExecutorID execId;
				ExecutorStateTag stateTag;
				std::shared_ptr<IExecutor> pExecutor;

				ExecutorState(ExecutorID execId_,
					ExecutorStateTag stateTag_,
					std::shared_ptr<IExecutor> pExecutor_)
					:execId(execId_),
					stateTag(stateTag_),
					pExecutor(pExecutor_)
				{ }
			};

		public:
			ExecutorThread(ThreadID id,
				const std::string& threadName,
				ExecutorID firstExecId,
				std::shared_ptr<IExecutor> pFirstExecutor);

			void JoinMe();

			const char* GetThreadName() const
			{
				return m_threadName.c_str();
			}

			void PostAddExecutor(ExecutorID execId,
				std::shared_ptr<IExecutor> pExecutor);
			void PostSignalExecutor(ExecutorID execId,
				ExecutorSignalType signalType);

			// Post a signal to all executors on this thread
			void PostSignalAll(ExecutorSignalType signalType);
			void DoomMe();
		private:

			void operator()(ExecutorID firstExecId, 
				std::shared_ptr<IExecutor> pFirstExecutor);

			void AddExecutor(ExecutorID execId, 
				std::shared_ptr<IExecutor> pExecutor);
			void HandleExecutorSignal(ExecutorID execId, ExecutorSignalType signalType);

			void HandleSignalForIndex(size_t execIndex, ExecutorSignalType signalType);

			// Remove executor from the array
			void RemoveExecutor(ExecutorID execId);

			// Thread
			ThreadID m_threadId;
			std::string m_threadName;
			std::thread m_thread;

			// Messaging
			std::atomic<bool>   m_newInstructions{ false };
			std::mutex m_mutexInstructions;
			std::condition_variable m_cvInstructions;
			std::vector<ExecutionInstructionMessage>  m_instructionList;
			bool  m_isDoomed{ false };

			// Once this goes to zero, the thread goes to sleep waiting for someone to 
			// be signalled
			unsigned int m_nActive;

			// Map for signalling.  Local only
			std::unordered_map<ExecutorID, size_t>  m_signalMap;

			// When this becomes empty, the thread removes itself from the owner's
			// map and ends.  This is the cleanest way.  Short-term tasks should not
			// be scheduled as executors
			std::vector<ExecutorState> m_executors;
		};

	public:
		static ExecutionManager& GetInstance();

		ExecutorID AddExecutor(const char* pThreadName,
			const std::shared_ptr<IExecutor>& pExecutor);

		// "Lock up shop", disallowing new executors and optionally sending a signal for existing
		// executors to stop executing
		void LockShop(bool sendTermination);

		// A "doomed" thread is no longer associated with its name, and it no longer sleeps waiting
		// for new executors when its executors run out
		void DoomThread(const char* pThreadName);

		bool SignalExecutor(ExecutorID executorId, ExecutorSignalType signalType);

		// Wait for all threads to terminate -- the "run loop"
		// Returns when all threads are joinable.
		void RunLoop(std::chrono::microseconds wakeUpTime);

	private:
		ExecutionManager();
		~ExecutionManager();

		const std::chrono::microseconds& GetIdleTimeout() const { return m_idleTimeout; }
		// Called only by inner-class objects
		void RemoveThread(ThreadID threadId);
		void RemoveExecutors(const std::vector<ExecutorID>& toRemove);

		// TODO:  Since "signal" may be called quite often, is there a better way to associate
		// executor IDs to threads?
		std::shared_mutex m_mutex;
		// NOTE:  This class is of course slower than std::condition_variable
		// But the overhead is fairly seldom
		std::condition_variable_any m_cvExecution;

		std::unordered_map<ExecutorID, ThreadID> m_executorMap;
		std::unordered_map<std::string, ThreadID>  m_threadNameMap;
		std::unordered_map<ThreadID, std::shared_ptr<ExecutorThread> >  m_threadMap;

		ThreadID m_nextThread{ 0 };
		ExecutorID m_nextExec{ 0 };

		bool m_lockedShop{ false };
		std::chrono::microseconds m_idleTimeout;

		std::vector<std::shared_ptr<ExecutorThread> >  m_threadsToJoin;
	};

}