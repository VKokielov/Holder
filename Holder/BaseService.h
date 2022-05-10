#pragma once

#include "IService.h"
#include "ExecutionManager.h"
#include "Messaging.h"
#include "SharedObjects.h"
#include "SharedObjectStore.h"
#include "StartupTaskManager.h"

#include <type_traits>
#include <memory>
#include <mutex>
#include <sstream>

namespace holder::service
{

	template<typename Dispatcher,
			 typename Base>
	class BaseService : public Dispatcher,
		public Base,
		public std::enable_shared_from_this<BaseService<Dispatcher> > ,
		public base::startup::ITaskStateListener
	{
	private:
		using ShEnabler = std::enable_shared_from_this<BaseService<Dispatcher> >;

		static_assert(std::is_base_of_v<messages::IMessageDispatcher, Dispatcher>,
			"Dispatcher must implement messages::IMessageDispatcher");
		static_assert(std::is_base_of_v<base::IExecutor, Dispatcher>,
			"Dispatcher must implement base::IExecutor");
		static_assert(std::is_base_of_v<IService, Base>,
			"Base must inherit from IService");

		struct Dependency_
		{
			std::string depPath;
			base::StoredObjectID depObjectId;

			Dependency_(const char* pPath, base::StoredObjectID depObjectId_ = 0)
				:depPath(pPath),
				depObjectId(depObjectId_)
			{ }
		};

		static std::string GetReadyTaskName(const chaR* pServicePath)
		{
			std::stringstream ssmReadyTaskName;

			ssmReadyTaskName << pServicePath << "|ready";
			return ssmReadyTaskName.str();
		}

	protected:
		using DependencyID = size_t;

		DependencyID AddDependency(const char* pDependencyPath)
		{
			// Add a dependency here
			// For the startup manager, the dependencies will be set when StartService() is called
			std::unique_lock lk{ m_stateMutex };
			
			m_dependencies.emplace_back(pDependencyPath);

			return m_dependencies.size() - 1;
		}

		void SetAutoCompleteStartup(bool flag)
		{
			m_autoCompleteStartup = flag;
		}

		// Do not call this function regularly, because it takes the mutex
		// It is for convenience and intended to be syntactic sugar
		base::SharedObjectPtr GetDependencyPtr(DependencyID depId)
		{
			base::SharedObjectPtr depPtr;
			std::string depPath;
			{
				std::unique_lock lk{ m_stateMutex };
				if (depId < m_dependencies.size())
				{
					depPath = m_dependencies[depId].depPath;
				}
				else
				{
					return depPtr;
				}
			}

			auto getObject = [&depPtr](base::StoredObjectID objId,
				const base::SharedObjectPtr& pObjPtr)
			{
				depPtr = pObjPtr;
			};
					
			base::SharedObjectStore::FindObject(depPath.c_str(), getObject);
			
			return depPtr;
		}

		void MarkReady(const std::shared_ptr<base::startup::ITaskResult> pResult)
		{
			std::unique_lock lk{ m_stateMutex };
			
			if (!m_completedStartup)
			{
				// Mark the task as complete and add the result as here given
				base::startup::StartupTaskManager::GetInstance()
					.CompleteTask(m_readyTaskID, true, pResult);

				m_completedStartup = true;
			}
		}

	public:
		BaseService(const IServiceConfiguration& myConfig)
			:Dispatcher(myConfig->GetServiceThreadName(), myConfig->TraceLostMessages()),
			m_pConfig(myConfig.Clone()),
			m_servicePath(myConfig->GetServicePath())
		{
			std::string strReadyTaskName = GetReadyTaskName(m_servicePath.c_str());
			m_readyTaskID = 
				base::startup::StartupTaskManager::GetInstance().DefineStartupTask(strReadyTaskName.c_str(),
				ShEnabler::shared_from_this());
		}

		// IStartupTaskListener
		void OnTaskReady(base::startup::StartupTaskID taskId, 
			base::startup::ITaskStateAccessor& taskStates) override
		{
			// All dependencies are ready.  Start the executor.  
			// Only after the executor has started do we mark the sservice as ready
			std::unique_lock lk{ m_stateMutex };
			if (m_readyTaskID == taskId)
			{
				Dispatcher::InitExecutor();
			}
		}
		
		// If the "ready" task fails for any dependency, the default behavior
		// is to fail this service as well.  Alternatives -- such as new dependencies --
		// can be implemented.  It must be remembered that this code is called from "some" thread

		void OnDependencyFailed(base::startup::StartupTaskID taskId,
			const char* dependencyName,
			base::startup::ITaskStateAccessor& taskStates) override
		{
			std::unique_lock lk{ m_stateMutex };
			if (m_readyTaskID == taskId)
			{
				if (!m_failedStartup)
				{
					m_failedStartup = true;
					base::startup::StartupTaskManager::GetInstance().CompleteTask(m_readyTaskId, false);
				}
			}
		}

		void OnRequestFailed(base::startup::StartupTaskID taskId,
			base::startup::RequestType requestType,
			base::startup::RequestFailType failType) override
		{

		}

		// IService
		void StartService() override
		{
			// Set the dependencies for the startup manager
			// NOTE:  Until SetDependencies has been called, there is no chance that a given task
			// will be marked as complete
			// Therefore there is no race in any sense
			std::unique_lock lk{ m_stateMutex };
			std::vector<std::string> depVector;

			for (const Dependency_& dep : m_dependencies)
			{
				depVector.emplace_back(GetReadyTaskName(dep.depPath));
			}

			// An empty dependency vector is also valid and means that the task
			// is marked as "ready" at once or asap
			base::startup::StartupTaskManager::GetInstance().SetStartupDependencies(m_readyTaskID, depVector);
		}

		// Should be defined in Dispatcher.  Called when the executor has started,
		// and marks the "ready" task as complete
		void Init() override
		{
			Dispatcher::Init();

			std::unique_lock lk{ m_stateMutex };
			
			if (m_autoCompleteStartup)
			{
				lk.unlock();
				// Mark the task as complete without any result 
				MarkReady(std::shared_ptr<base::startup::ITaskResult>());
			}

		}

	private:
		std::shared_ptr<IServiceConfiguration> m_pConfig;
		std::string m_servicePath;
		std::vector<Dependency_> m_dependencies;
		bool m_autoCompleteStartup{ false };

		std::mutex m_stateMutex;
		base::startup::StartupTaskID m_readyTaskID;
		bool m_failedStartup{ false };
		bool m_completedStartup(false);

	};




}