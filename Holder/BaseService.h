#pragma once

#include "IService.h"
#include "ExecutionManager.h"
#include "Messaging.h"
#include "SharedObjectStore.h"
#include "StartupTaskManager.h"
#include "TypeTagDisp.h"
#include "BaseServiceObject.h"

#include <type_traits>
#include <memory>
#include <mutex>
#include <sstream>

namespace holder::service
{

	template<typename Derived,
		typename ... Bases>
	class BaseService : public BaseServiceObject<Derived, Bases...>,
		public IService,
		public base::startup::ITaskStateListener
	{
	private:

		struct Dependency_
		{
			std::string depPath;
			base::StoredObjectID depObjectId;

			Dependency_(const char* pPath, base::StoredObjectID depObjectId_ = 0)
				:depPath(pPath),
				depObjectId(depObjectId_)
			{ }
		};

		static std::string GetReadyTaskName(const char* pServicePath)
		{
			std::stringstream ssmReadyTaskName;

			ssmReadyTaskName << pServicePath << "|ready";
			return ssmReadyTaskName.str();
		}

	protected:
		using DependencyID = size_t;

		virtual std::shared_ptr<base::startup::ITaskStateListener>
			GetMyTaskStateListenerSharedPtr() = 0;

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
				return true;
			};
					
			base::SharedObjectStore::GetInstance().FindObject(depPath.c_str(), getObject);
			
			return depPtr;
		}

		void MarkReady(const std::shared_ptr<base::startup::ITaskResult> pResult)
		{
			std::unique_lock lk{ m_stateMutex };
			
			if (!m_completedStartup && !m_failedStartup)
			{
				// Mark the task as complete and add the result as here given
				base::startup::StartupTaskManager::GetInstance()
					.CompleteTask(m_readyTaskID, true, pResult);

				m_completedStartup = true;
			}
		}

		void MarkFailed()
		{
			std::unique_lock lk{ m_stateMutex };

			if (!m_failedStartup && !m_completedStartup)
			{
				// Mark the task as complete and add the result as here given
				base::startup::StartupTaskManager::GetInstance()
					.CompleteTask(m_readyTaskID, false);

				m_failedStartup = true;
			}
		}

		void OnDependenciesAdded()
		{
			// Set the dependencies for the startup manager
			// NOTE:  Until SetDependencies has been called, there is no chance that a given task
			// will be marked as complete
			// Therefore there is no race in any sense
			std::unique_lock lk{ m_stateMutex };
			std::vector<std::string> depVector;

			for (const Dependency_& dep : m_dependencies)
			{
				depVector.emplace_back(GetReadyTaskName(dep.depPath.c_str()));
			}

			// An empty dependency vector is also valid and means that the task
			// is marked as "ready" at once or asap
			base::startup::StartupTaskManager::GetInstance().SetStartupDependencies(m_readyTaskID, depVector);
		}

		BaseService(const IServiceConfiguration& myConfig)
			:Dispatcher(myConfig.GetServiceThreadName(), myConfig.TraceLostMessages()),
			m_pConfig(static_cast<IServiceConfiguration*>(myConfig.Clone())),
			m_servicePath(myConfig.GetServicePath())
		{

		}

		void AddService()
		{
			std::string strReadyTaskName = GetReadyTaskName(m_servicePath.c_str());
			m_readyTaskID =
				base::startup::StartupTaskManager::GetInstance().DefineStartupTask(strReadyTaskName.c_str(),
					GetMyTaskStateListenerSharedPtr());
		}

	public:

		void OnCreated() override
		{
			AddService();
			OnDependenciesAdded();
		}

		// ***ITaskStateListener*** for startup
		void OnTaskReady(base::startup::StartupTaskID taskId,
			base::startup::ITaskStateAccessor& taskStates) override
		{
			// All dependencies are ready.  Add the service to the queue manager by creating
			// a default receiver

			std::unique_lock lk{ m_stateMutex };
			if (m_readyTaskID == taskId)
			{
				// Create a default receiver (adding myself to my queue and causing a callback
				// when the queue first executes)
				messages::BaseMessageHandler<Derived,Bases...>::CreateDefaultReceiver();

				// Set task to running
				base::startup::StartupTaskManager::GetInstance().RunTask(m_readyTaskID);
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
					base::startup::StartupTaskManager::GetInstance().CompleteTask(m_readyTaskID, false);
				}
			}
		}

		void OnReceiverReady(messages::DispatchID dispatchId)  override
		{
			std::unique_lock lk{ m_stateMutex };
			
			if (m_autoCompleteStartup)
			{
				lk.unlock();
				// Mark the task as complete without any result 
				MarkReady(std::shared_ptr<base::startup::ITaskResult>());
			}

			return true;
		}

		void OnRequestFailed(base::startup::StartupTaskID taskId,
			base::startup::RequestType requestType,
			base::startup::RequestFailType failType) override
		{ }

	private:
		std::shared_ptr<IServiceConfiguration> m_pConfig;
		std::string m_servicePath;
		std::vector<Dependency_> m_dependencies;
		bool m_autoCompleteStartup{ false };

		std::mutex m_stateMutex;
		base::startup::StartupTaskID m_readyTaskID;
		bool m_failedStartup{ false };
		bool m_completedStartup{ false };

	};

}