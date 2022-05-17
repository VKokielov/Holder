#pragma once

#include "ExecutionManager.h"
#include "StartupTaskManagerInterfaces.h"

#include <cinttypes>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <deque>

#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace holder::base::startup
{

	class StartupTaskManager
	{
	private:
		class StartupTaskExecutor;
		class DefineTaskAction;
		class SetDependenciesAction;
		class RunTaskAction;
		class CompleteTaskAction;


		// These are sent to the executor and act there
		class IStartupAction
		{
		public:
			virtual void Act(StartupTaskExecutor* pExecutor) const = 0;
			virtual ~IStartupAction() = default;
		};


		class StartupTaskExecutor : public IExecutor, public ITaskStateAccessor
		{
		private:
			struct TaskStateDesc_
			{
				StartupTaskID id;
				std::string name;
				TaskState state{ TaskState::Undefined };
				std::shared_ptr<ITaskStateListener> pListener;
				std::shared_ptr<ITaskResult> pResult;

				std::unordered_set<std::string> dependsOnUndefined;
				std::unordered_set<StartupTaskID> dependsOn;
				std::unordered_set<StartupTaskID> dependedOnBy;

				std::unordered_set<StartupTaskID> failuresReported;
			};

			struct TaskNameDesc_
			{
				std::optional<StartupTaskID> taskId;
				// This set is only there for undefined tasks
				std::optional<std::unordered_set<StartupTaskID> > dependedOnBy;
			};

		public:
			StartupTaskExecutor(bool stopWhenComplete)
				:m_stopWhenComplete(stopWhenComplete)
			{ }
			bool Init() override;
			void DeInit() override;
			ExecutionState Run(ExecutionArgs& args) override;
			void TerminationRequested() override;

			std::shared_ptr<ITaskResult>
				GetTaskResult(const char* taskName) override;
			TaskState GetTaskState(const char* taskName) override;

			void HandleAction(const DefineTaskAction& action);
			void HandleAction(const SetDependenciesAction& action);
			void HandleAction(const RunTaskAction& action);
			void HandleAction(const CompleteTaskAction& action);

		private:
			void EvaluateDependencies(TaskStateDesc_& stateDesc);

			std::unordered_map<std::string, TaskNameDesc_>  m_taskNameMap;
			std::unordered_map<StartupTaskID, TaskStateDesc_> m_taskStates;

			std::deque<std::shared_ptr<IStartupAction> >
				m_localQueue;
			bool m_stopWhenComplete{ false };
			bool m_stopRequested{ false };

			unsigned int m_waitingTasks{ 0 };
		};

		template<typename D>
		class StartupActionBase : public IStartupAction
		{
		public:
			void Act(StartupTaskExecutor* pExecutor) const override
			{
				pExecutor->HandleAction(static_cast<const D&>(*this));
			}

			StartupTaskID GetID() const { return m_id; }

		protected:
			StartupActionBase(StartupTaskID id)
				:m_id(id)
			{ }

		private:
			StartupTaskID m_id;
		};

		class DefineTaskAction : public StartupActionBase<DefineTaskAction>
		{
		public:
			DefineTaskAction(StartupTaskID taskId, const std::string& name,
				std::shared_ptr<ITaskStateListener> pListener)
				:StartupActionBase<DefineTaskAction>(taskId),
				m_name(name),
				m_pListener(pListener)
			{ }

			const std::string& GetName() const { return m_name; }
			const std::shared_ptr<ITaskStateListener>& GetListener() const { return m_pListener; }

		private:
			std::string m_name;
			std::shared_ptr<ITaskStateListener>  m_pListener;
		};

		class SetDependenciesAction : public StartupActionBase<SetDependenciesAction>
		{
		public:
			SetDependenciesAction(StartupTaskID taskId, std::unordered_set<std::string> depNames)
				:StartupActionBase<SetDependenciesAction>(taskId),
				m_depNames(std::move(depNames))
			{ }

			const std::unordered_set<std::string>& GetDeps() const { return m_depNames; }
		private:
			std::unordered_set<std::string> m_depNames;
		};

		class RunTaskAction : public StartupActionBase<RunTaskAction>
		{
		public:
			RunTaskAction(StartupTaskID taskId)
				:StartupActionBase<RunTaskAction>(taskId)
			{ }
		};

		class CompleteTaskAction : public StartupActionBase<CompleteTaskAction>
		{
		public:
			CompleteTaskAction(StartupTaskID taskId, bool success,
				std::shared_ptr<ITaskResult> pResult)
				:StartupActionBase<CompleteTaskAction>(taskId),
				m_success(success),
				m_pResult(pResult)
			{ }

			bool GetSuccess() const { return m_success; }
			const std::shared_ptr<ITaskResult> GetResult() const { return m_pResult; }
		private:
			bool m_success;
			std::shared_ptr<ITaskResult>  m_pResult;
		};
		
	public:

		StartupTaskID DefineStartupTask(const char* taskName, 
			std::shared_ptr<ITaskStateListener> pListener);
		void SetStartupDependencies(StartupTaskID taskId, const std::vector<std::string>& deps);
		void RunTask(StartupTaskID taskId);
		void CompleteTask(StartupTaskID taskId, bool success, std::shared_ptr<ITaskResult> pResult 
															= std::shared_ptr<ITaskResult>());

		// If "stopWhenComplete" is true, the executor will stop as soon as all dependencies are resolved
		// This should be set to "true" unless one expects to add new components dynamically
		// Setting this to false does not affect termination requests, which are honored immediately
		void StartExecutor(bool stopWhenComplete);

		static StartupTaskManager& GetInstance();

	private:
		// Diagnostics
		void OnInvalidTaskID(StartupTaskID taskId, RequestType requestType);

		std::mutex m_mutex;

		std::shared_ptr<StartupTaskExecutor> m_pMyExecutor;
		ExecutorID m_execID;
		bool m_running{ false };
		StartupTaskID m_nextTaskId{ 0 };
		std::deque<std::shared_ptr<IStartupAction> > m_executorQueue;
	};


}