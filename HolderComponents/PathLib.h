#pragma once

#include "ObjectTree.h"
#include <algorithm>
#include <memory>

namespace holder::lib
{
	// Add a node, optionally creating directories on the way

	enum class TraceAction
	{
		OK,
		StopTrace,
		FindChild,
		CreateDictionary,
		CreateValue
	};

	// Linear trace
	template<typename Tree,
		typename Path, typename F>
	NodeResult TracePathGeneric(Tree& tree,
		const Path& path,
		F&& callback,
		NodeID startId = NODE_ROOT)
	{
		constexpr bool is_const = std::is_const_v<std::remove_reference_t<Tree> >;

		auto itPath = path.begin();

		NodeID nodeId = path.rooted() || startId == NODE_ROOT
			? tree.GetRoot() : startId;

		size_t pathSize = path.size();
		size_t pathIdx{ 0 };

		bool wasCreated{ false };

		while (itPath != path.end())
		{
			auto nextAction =
				callback(tree, pathIdx, pathSize, nodeId, wasCreated);
			wasCreated = false;  // for next iteration

			if (nextAction == TraceAction::StopTrace)
			{
				return NodeResult::VisitorError;
			}

			bool createChild = nextAction == TraceAction::CreateDictionary
				|| nextAction == TraceAction::CreateValue;

			// Safe to pass in nodeId by reference via the third argument since the first is by-value
			NodeResult childResult;
			if (createChild)
			{
				if constexpr (!is_const)
				{
					childResult = tree.AddNode(nodeId, *itPath,
						nextAction == TraceAction::CreateDictionary ? NodeType::Dictionary : NodeType::Value,
						nodeId);

					if (childResult == NodeResult::OK)
					{
						wasCreated = true;
					}
					else if (childResult != NodeResult::DuplicateName)
					{
						return childResult;
					}
				}
				else
				{
					return NodeResult::VisitorError;
				}
			}
			else
			{
				childResult = tree.FindChild(nodeId, *itPath, nodeId);
				if (childResult != NodeResult::OK)
				{
					return childResult;
				}
			}

			++pathIdx;
			++itPath;
		}

		// Visit the last entry
		if (callback(tree, pathIdx, pathSize, nodeId, wasCreated) != TraceAction::OK)
		{
			return NodeResult::VisitorError;
		}

		return NodeResult::OK;
	}

	template<typename Value, typename Metadata,
		typename Path, typename F>
	NodeResult TracePath(ObjectTree<Value, Metadata>& tree,
		const Path& path,
		F&& callback,
		NodeID startId = NODE_ROOT)
	{
		return TracePathGeneric(tree, path, std::forward<F>(callback), startId);
	}

	template<typename Value, typename Metadata,
		typename Path, typename F>
	NodeResult TracePath(const ObjectTree<Value, Metadata>& tree,
		const Path& path,
		F&& callback,
		NodeID startId = NODE_ROOT)
	{
		return TracePathGeneric(tree, path, std::forward<F>(callback), startId);
	}

	template<typename Path>
	bool PathStartsWith(const Path& path,
		const Path& prefix)
	{
		if (path.size() < prefix.size())
		{
			return false;
		}
		auto cmpStrings = [](const char* pLeft, const char* pRight)
		{
			return pLeft && pRight && !strcmp(pLeft, pRight);
		};

		return std::mismatch(prefix.begin(), prefix.end(), path.begin(), cmpStrings).first
			== prefix.end();
	}

	// Visitors
	template<typename Value, typename Metadata = typename ObjectTree<Value>::TMetadata>
	class NodeAdder
	{
	public:
		NodeAdder(bool createDirs)
			:m_createDirs(createDirs)
		{

		}

		TraceAction operator () (ObjectTree<Value, Metadata>& tree,
			size_t pathIdx,
			size_t pathSize,
			NodeID nodeId,
			bool wasCreated)
		{
			if (pathIdx < pathSize - 1)
			{
				if (m_createDirs)
				{
					return TraceAction::CreateDictionary;
				}
				else
				{
					return TraceAction::FindChild;
				}
			}
			else if (pathIdx == pathSize - 1)
			{
				return TraceAction::CreateValue;
			}

			m_nodeId = nodeId;
			// Otherwise we would expect the last value to have been created
			return wasCreated ? TraceAction::OK : TraceAction::StopTrace;
		}

		NodeID GetNodeID() const
		{
			return m_nodeId;
		}
	private:
		NodeID m_nodeId;
		bool m_createDirs;
	};

	template<typename Value, typename Metadata = typename ObjectTree<Value>::TMetadata>
	class NodeFinder
	{
	public:
		TraceAction operator () (const ObjectTree<Value, Metadata>& tree,
			size_t pathIdx,
			size_t pathSize,
			NodeID nodeId,
			bool wasCreated)
		{
			if (pathIdx < pathSize)
			{
				return TraceAction::FindChild;
			}

			m_nodeId = nodeId;
			return TraceAction::OK;
		}

		NodeID GetNodeID() const
		{
			return m_nodeId;
		}
	private:
		NodeID m_nodeId;
	};

	class PathFromString
	{
	public:
		PathFromString(const char* pPath, char sep = '/');

		bool rooted() const { return m_pathBuffer[0] == m_sep; }
		auto begin() const
		{
			return m_analyzedPath.cbegin();
		}
		auto end() const
		{
			return m_analyzedPath.cend();
		}

		size_t size() const { return m_analyzedPath.size(); }
	private:
		std::vector<const char*> m_analyzedPath;
		std::unique_ptr<char[]> m_pathBuffer;
		char m_sep;
	};
}