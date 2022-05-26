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
#include <queue>

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

		enum class TimerInstructionTag
		{
			Set,
			Cancel
		};

		struct ExecutionInstructionMessage
		{
			ExecutionInstructionTag instructionTag;
			ExecutorSignalType signalType;
			ExecutorID execId;
			std::shared_ptr<IExecutor> executor;
		};

		struct TimerDefinition
		{
			std::string timerName;
			typename std::chrono::steady_clock::duration timeInterval;
			bool repeatingTimer;
			TimerUserID timerUserID;
			std::shared_ptr<ITimerCallback> pCallback;
		};

		struct TimerMessage
		{
			TimerInstructionTag tag;
			TimerDefinition timerDef;
			TimerID timerID;
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
				bool isSingleton);

			void StartMe(ExecutorID firstExecId,
				std::shared_ptr<IExecutor> pFirstExecutor);
			void JoinMe();

			const char* GetThreadName() const
			{
				return m_threadName.c_str();
			}

			bool IsSingleton() const { return m_isSingleton; }

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
			bool m_isSingleton;

			std::thread m_thread;

			// Messaging
			std::atomic<bool>   m_newInstructions{ false };
			std::mutex m_mutexInstructions;
			std::condition_variable m_cvInstructions;
			std::vector<ExecutionInstructionMessage>  m_instructionList;
			bool  m_isDoomed{ false };

			// Once this goes to zero, the thread goes to sleep waiting for someone to 
			// be signalled
			unsigned int m_nActive{ 0 };

			// Map for signalling.  Local only
			std::unordered_map<ExecutorID, size_t>  m_signalMap;

			// When this becomes empty, the thread removes itself from the owner's
			// map and ends.  This is the cleanest way.  Short-term tasks should not
			// be scheduled as executors
			std::vector<ExecutorState> m_executors;
		};

		// TODO: If lookup by name is too slow, consider introucing a real ID

		// For the sake of simplicity, only new timers are inserted into the priority queue
		// during processing of updates
		// Other timers are left alone to fire.
		// This prevents unexpected upheavals of the queue.

		// To get around this, remove and add a timer.  The old timer will remain in the priority queue
		// but will be ignored and finally removed when its turn comes.

		class TimerThread
		{
		private:
			struct TimerPriorityEntry
			{
				std::chrono::time_point<std::chrono::steady_clock> nextBeat;
				size_t timerIndex;

				bool operator < (const TimerPriorityEntry& other) const
				{
					// Priority queues are sorted "largest-first" so we need to reverse the sense
					return nextBeat > other.nextBeat;
				}
			};

			struct TimerStateWrapper
			{
				// Set to true when the timer arguments were changed
				// Currently used to prevent immediate removal of one-shot timers when
				// they are recalibrated
				bool recalibrate;
				TimerDefinition timerDef;
			};

			enum class TimerAction
			{
				Reinsert,
				Remove
			};


		public:
			TimerThread();

			void StartMe();
			void DoomMe();
			void JoinMe();

			void SetTimer(const char* pTimerName,
				unsigned long microInterval,
				bool repeatingTimer,
				TimerUserID timerUserID,
				std::shared_ptr<ITimerCallback> pCallback);
			
			void CancelTimer(const char* pTimerName);
			void CancelTimer(TimerID timerID);
		private:
			void ProcessTimerMessage(const TimerMessage& msg);

			// Called when a timer is "due" in the sense that it is past its deadline
			// dataLock is provided so the mutex can be unlocked temporarily while 
			// making the callback
			TimerAction OnTimerDue(TimerPriorityEntry& timerEntry);

			void operator()();

			std::thread m_thread;

			// NOTE:  This mutex protects ONLY the deque!
			// However, the wait function uses the top of the priority queue, which is local
			std::mutex m_mutexInstructions;
			std::condition_variable m_cvInstructions;

			std::deque<TimerMessage> m_messages;

			std::unordered_map<TimerID, TimerStateWrapper> m_timerStates;
			std::priority_queue<TimerPriorityEntry> m_timerPriorities;
			TimerID m_nextTimerIndex{ 0 };

			// Atomic here
			std::atomic<bool>  m_threadDoomed;
		};


	public:
		static ExecutionManager& GetInstance();
		static const std::string& GetCurrentThreadName()
		{
			return m_tlThreadName;
		}

		ExecutorID AddExecutor(const char* pThreadName,
			const std::shared_ptr<IExecutor>& pExecutor,
			bool isSingleton = false);

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

		// Timer creation
		bool SetTimer(const char* pTimerName,
			unsigned long microInterval,
			bool repeatingTimer,
			TimerID timerID,
			std::shared_ptr<ITimerCallback> pCallback);

		void CancelTimer(const char* pTimerName);
		void CancelTimer(TimerID timerID);

	private:
		ExecutionManager();
		~ExecutionManager();

		const std::chrono::microseconds& GetIdleTimeout() const { return m_idleTimeout; }
		// Called only by inner-class objects
		void RemoveThread(ThreadID threadId);
		void RemoveExecutors(const std::vector<ExecutorID>& toRemove);
		void LockedShopUpdateState();

		// TODO:  Since "signal" may be called quite often, is there a better way to associate
		// executor IDs to threads?
		std::shared_mutex m_mutex;
		// NOTE:  This class is of course slower than std::condition_variable
		// But the overhead is fairly seldom
		std::condition_variable_any m_cvExecution;

		std::unordered_map<ExecutorID, ThreadID> m_executorMap;
		std::unordered_map<std::string, ThreadID>  m_threadNameMap;
		std::unordered_map<ThreadID, std::shared_ptr<ExecutorThread> >  m_threadMap;

		std::unique_ptr<TimerThread> m_pTimerThread;

		ThreadID m_nextThread{ 0 };
		ExecutorID m_nextExec{ 0 };

		bool m_lockedShop{ false };
		std::chrono::microseconds m_idleTimeout;

		std::vector<std::shared_ptr<ExecutorThread> >  m_threadsToJoin;

		static thread_local std::string m_tlThreadName;
	};

}