#pragma once

#include <cinttypes>
#include <unordered_map>

namespace holder::lib
{

	enum class NodeType
	{
		None,
		Dictionary,
		Value
	};

	enum class NodeResult
	{
		OK,
		InvalidNodeID,
		NeedDictionary,   // for example when creating or finding children
		NeedValue,
		DuplicateName,
		NameNotFound,
		RootInvalid,  // Can't delete root
		DictEmptyRequired, // can only delete empty dictionaries to avoid orphaned child nodes
		VisitorError // some function returned false during a visitor callback
	};

	using NodeID = uint64_t;
	constexpr NodeID NODE_ROOT = 0;

	struct DefaultMetadata { };

	template<typename Val, typename Metadata = DefaultMetadata>
	class ObjectTree
	{
	public:
		using TMetadata = Metadata;
	private:
		// The renaming is cosmetic.  No type except uint64_t will work, due
		// to the way IDs are analyzed
		static_assert(std::is_same_v<NodeID, uint64_t>,
			"NodeID must be a 64-bit value");

		static void AnalyzeID(NodeID inId,
			NodeType& nodeType,
			size_t& outIdx)
		{
			if (inId & ((uint64_t)1 << 32))
			{
				nodeType = NodeType::Value;
				outIdx = inId & 0xffffffff;
			}
			else
			{
				nodeType = NodeType::Dictionary;
				outIdx = inId;
			}
		}

		static NodeID SynthesizeID(NodeType nodeType,
			size_t idx)
		{
			if (nodeType == NodeType::Value)
			{
				return ((uint64_t)1 << 32) | idx;
			}
			return idx;
		}

		class Node
		{
		public:
			NodeType GetNodeType() const { return m_nodeType; }
			NodeID GetParentID() const { return m_parentId; }
			NodeID GetID() const { return m_myId; }
			Metadata& GetMetadata() { return m_metaData; }
			const Metadata& GetMetadata() const { return m_metaData; }

		protected:
			Node(ObjectTree& owner, NodeID myId, NodeID parentId, NodeType nodeType)
				:m_owner(owner),
				m_myId(myId),
				m_parentId(parentId),
				m_nodeType(nodeType)
			{ }

			ObjectTree& GetOwner() { return m_owner; }
		private:
			ObjectTree& m_owner;
			NodeID m_myId;
			NodeID m_parentId;
			NodeType m_nodeType;
			Metadata m_metaData;
		};

		class DictionaryNode : public Node
		{
		public:
			DictionaryNode(ObjectTree& owner, NodeID myId, NodeID parentId)
				:Node(owner, myId, parentId, NodeType::Dictionary)
			{ }

			bool FindChild(const char* pName, NodeID& nodeId) const
			{
				auto itNode = m_childNodes.find(std::string(pName));
				if (itNode != m_childNodes.end())
				{
					nodeId = itNode->second;
					return true;
				}

				return false;
			}

			bool CreateChild(const char* pName, 
				NodeType nodeType,
				NodeID& nodeId)
			{
				auto emplResult = m_childNodes.emplace(std::string(pName), NODE_ROOT);
				if (!emplResult.second)
				{ 
					// Already exists
					nodeId = emplResult.first->second;
					return false;
				}

				nodeId = Node::GetOwner().CreateNode(Node::GetID(), nodeType);

				emplResult.first->second = nodeId;
				return true;
			}

			bool IsEmpty() const { return m_childNodes.empty(); }

			void RemoveChild(NodeID childId)
			{
				for (auto itChild = m_childNodes.begin();
					itChild != m_childNodes.end();
					++itChild)
				{
					if (itChild->second == childId)
					{
						m_childNodes.erase(itChild);
						break;
					}
				}
			}
		private:
			std::unordered_map<std::string, NodeID> m_childNodes;
		};

		class ValueNode : public Node
		{
		public:
			ValueNode(ObjectTree& owner, NodeID myId, NodeID parentId)
				:Node(owner, myId, parentId, NodeType::Value)
			{ }

			void Set(const Val& val)
			{
				m_value = val;
			}

			const Val& Get() const
			{
				return m_value;
			}
		private:
			Val m_value;
		};

	public:

		ObjectTree()
		{
			// Create the root (dictionary) node
			m_dictNodes.emplace(std::piecewise_construct,
				std::forward_as_tuple(0),
				std::forward_as_tuple(*this, NODE_ROOT, NODE_ROOT));
			++m_nextDict;
		}

		NodeResult AddNode(NodeID parentId,
			const char* pNodeName,
			NodeType nodeType,
			NodeID& resultId)
		{
			NodeType parentType;
			DictionaryNode* pNode =
				static_cast<DictionaryNode*>(GetNode(parentId, parentType));

			if (!pNode)
			{
				return NodeResult::InvalidNodeID;
			}

			if (parentType != NodeType::Dictionary)
			{
				return NodeResult::NeedDictionary;
			}

			if (!pNode->CreateChild(pNodeName, nodeType, resultId))
			{
				// Returns the existing node
				return NodeResult::DuplicateName;
			}

			return NodeResult::OK;
		}

		NodeResult RemoveNode(NodeID nodeId, bool updateParent)
		{
			if (nodeId == NODE_ROOT)
			{
				return NodeResult::RootInvalid;
			}

			NodeType nodeType;
			size_t nodeIdx;
			AnalyzeID(nodeId, nodeType, nodeIdx);

			NodeResult nodeResult{};
			if (nodeType == NodeType::Dictionary)
			{
				nodeResult = RemoveNodeInternal(m_dictNodes, nodeIdx, updateParent);
			}
			else
			{
				nodeResult = RemoveNodeInternal(m_valNodes, nodeIdx, updateParent);
			}

			return nodeResult;
		}

		bool IsEmpty(NodeID nodeId) const
		{
			NodeType nodeType;
			const Node* pNode = GetNode(nodeId, nodeType);

			return pNode && nodeType == NodeType::Dictionary
				&& static_cast<const DictionaryNode*>(pNode)->IsEmpty();
		}

		NodeResult GetValue(NodeID nodeId, Val& outValue) const
		{
			NodeType nodeType;
			const ValueNode* pNode =
				static_cast<const ValueNode*>(GetNode(nodeId, nodeType));

			if (!pNode)
			{
				return NodeResult::InvalidNodeID;
			}

			if (nodeType != NodeType::Value)
			{
				return NodeResult::NeedValue;
			}

			outValue = pNode->Get();
			return NodeResult::OK;
		}

		NodeResult SetValue(NodeID nodeId, const Val& value)
		{
			NodeType nodeType;
			ValueNode* pNode =
				static_cast<ValueNode*>(GetNode(nodeId, nodeType));

			if (!pNode)
			{
				return NodeResult::InvalidNodeID;
			}

			if (nodeType != NodeType::Value)
			{
				return NodeResult::NeedValue;
			}

			pNode->Set(value);
			return NodeResult::OK;
		}
		
		NodeResult FindChild(NodeID parentId, 
			const char* childName,
			NodeID& childId) const
		{
			NodeType parentType;
			const DictionaryNode* pNode = 
				static_cast<const DictionaryNode*>(GetNode(parentId, parentType));

			if (!pNode)
			{
				return NodeResult::InvalidNodeID;
			}
				
			if (parentType != NodeType::Dictionary)
			{
				return NodeResult::NeedDictionary;
			}

			if (!pNode->FindChild(childName, childId))
			{
				return NodeResult::NameNotFound;
			}

			return NodeResult::OK;
		}

		template<typename F>
		NodeResult VisitMetadata(NodeID nodeId, F&& visitor)
		{
			NodeType nodeType;
			Node* pNode = GetNode(nodeId, nodeType);

			if (!pNode)
			{
				return NodeResult::InvalidNodeID;
			}

			return visitor(pNode->GetMetadata()) ? NodeResult::OK : NodeResult::VisitorError;
		}

		template<typename F>
		NodeResult VisitMetadata(NodeID nodeId, F&& visitor) const
		{
			NodeType nodeType;
			const Node* pNode = GetNode(nodeId, nodeType);

			if (!pNode)
			{
				return NodeResult::InvalidNodeID;
			}

			return visitor(pNode->GetMetadata()) ? NodeResult::OK : NodeResult::VisitorError;
		}

		NodeID GetRoot() const
		{
			return NODE_ROOT;
		}

		NodeType GetNodeType(NodeID nodeId) const
		{
			// This is a "fair" check, not assuming that the node ID is valid
			NodeType nodeType;
			const Node* pNode = GetNode(nodeId, nodeType);

			if (!pNode)
			{
				return NodeType::None;
			}

			return nodeType;
		}

	private:
		Node* GetNode(NodeID nodeId, NodeType& nodeType)
		{
			size_t nodeIdx{ 0 };
			AnalyzeID(nodeId, nodeType, nodeIdx);
			Node* pRet{ nullptr };

			if (nodeType == NodeType::Dictionary)
			{
				auto itNode = m_dictNodes.find(nodeIdx);
				if (itNode != m_dictNodes.end())
				{
					pRet = &itNode->second;
				}
			}
			else if (nodeType == NodeType::Value)
			{
				auto itNode = m_valNodes.find(nodeIdx);
				if (itNode != m_valNodes.end())
				{
					pRet = &itNode->second;
				}
			}
			return pRet;
		}

		const Node* GetNode(NodeID nodeId, NodeType& nodeType) const
		{
			return const_cast<ObjectTree<Val,Metadata>*>(this)->GetNode(nodeId, nodeType);
		}

		NodeID CreateNode(NodeID parentId, NodeType nodeType)
		{
			NodeID retId{ NODE_ROOT };
			if (nodeType == NodeType::Dictionary)
			{
				auto nextDictID = m_nextDict;
				++m_nextDict;
				retId = SynthesizeID(nodeType, nextDictID);

				m_dictNodes.emplace(std::piecewise_construct,
					                std::forward_as_tuple(nextDictID),
					                std::forward_as_tuple(*this, retId, parentId));
			}
			else
			{
				auto nextValID = m_nextVal;
				++m_nextVal;

				retId = SynthesizeID(nodeType, nextValID);

				m_valNodes.emplace(std::piecewise_construct,
					std::forward_as_tuple(nextValID),
					std::forward_as_tuple(*this, retId, parentId));
			}
			return retId;
		}

		template<typename TNode>
		NodeResult RemoveNodeInternal(
			std::unordered_map<size_t, TNode>& map,
			size_t nodeIdx, 
			bool updateParent)
		{
			auto itNode = map.find(nodeIdx);

			if (itNode == map.end())
			{
				return NodeResult::InvalidNodeID;
			}

			if constexpr (std::is_same_v<TNode, DictionaryNode>)
			{
				if (!itNode->second.IsEmpty())
				{
					return NodeResult::DictEmptyRequired;
				}
			}

			if (updateParent)
			{
				NodeID parentId = itNode->second.GetParentID();
				NodeType parentType;
				size_t parentIndex;

				AnalyzeID(parentId, parentType, parentIndex);

				// The parent is of course a dictionary
				auto itParent = m_dictNodes.find(parentIndex);
				itParent->second.RemoveChild(nodeIdx);
			}
			map.erase(itNode);
			return NodeResult::OK;
		}

		std::unordered_map<size_t, DictionaryNode> m_dictNodes;
		size_t m_nextDict{ 0 };

		std::unordered_map<size_t, ValueNode> m_valNodes;
		size_t m_nextVal{ 0 };
	};


}