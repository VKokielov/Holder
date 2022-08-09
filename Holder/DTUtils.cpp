#include "DTUtils.h"

namespace impl_ns = holder::data;

bool impl_ns::SerializeDataTree(const std::shared_ptr<IDatum>& pRoot,
	ITreeSerializer& serializer)
{
	// This is implemented as a DFS on the tree.
	struct DFSLevel
	{
		struct Child
		{
			// The key is empty for lists
			std::string key{};
			std::shared_ptr<IDatum>  value;

			Child(const std::shared_ptr<IDatum>& pdatum)
				:value(pdatum)
			{ }

			Child(const char* pkey, const std::shared_ptr<IDatum>& pdatum)
				:key(pkey),
				value(pdatum)
			{ }
		};
		BaseDatumType type;
		std::shared_ptr<IDatum> level;
		std::vector<Child> children;
		size_t curIdx{ 0 };
		bool childrenLoaded{ false };

		DFSLevel(const std::shared_ptr<IDatum>& pLevel)
			:type(pLevel->GetDatumType()),
			level(pLevel)
		{ }

		void LoadChildren(const std::shared_ptr<IElementDatum>& pElementLevel)
		{ }

		void LoadChildren(const std::shared_ptr<IObjectDatum>& pObjectLevel)
		{ }

		void LoadChildren(const std::shared_ptr<IListDatum>& pListLevel)
		{
			// Get all children
			std::vector<std::shared_ptr<IDatum> > childNodes;
			pListLevel->GetRange(childNodes);

			for (auto& pDatum : childNodes)
			{
				children.emplace_back(pDatum);
			}
		}

		void LoadChildren(const std::shared_ptr<IDictDatum>& pDictLevel)
		{
			struct IterDict : public IDictCallback
			{
			public:
				IterDict(DFSLevel& owner_)
					:owner(owner_)
				{ }

				bool OnEntry(const char* pKey, const std::shared_ptr<IDatum>& pChild) override
				{
					owner.children.emplace_back(pKey, pChild);
					return true;
				}
			private:
				DFSLevel& owner;
			};

			IterDict dictIter(*this);
			pDictLevel->Iterate(dictIter);
		}

		void LoadChildren()
		{
			// If this function is NOT called then the children vector will be empty
			// and HasNextChild() will return false
			// This is used to implement IterInstruction::Skip in an intuitive way
			if (childrenLoaded)
			{
				return;
			}

			switch (type)
			{
			case BaseDatumType::Element:
				LoadChildren(std::static_pointer_cast<IElementDatum>(level));
				break;
			case BaseDatumType::Object:
				LoadChildren(std::static_pointer_cast<IObjectDatum>(level));
				break;
			case BaseDatumType::List:
				LoadChildren(std::static_pointer_cast<IListDatum>(level));
				break;
			case BaseDatumType::Dictionary:
				LoadChildren(std::static_pointer_cast<IDictDatum>(level));
				break;
			}

			childrenLoaded = true;
		}

		bool HasNextChild()
		{
			return curIdx < children.size();
		}

		const Child& NextChild()
		{
			const auto& rRet = children[curIdx];
			++curIdx;

			return rRet;
		}
	};

	std::vector<DFSLevel> dfsStack;
	dfsStack.emplace_back(pRoot);

	while (!dfsStack.empty())
	{
		IterInstruction iterIns{ IterInstruction::Enter };
		DFSLevel& curLevel = dfsStack.back();

		if (curLevel.curIdx == 0)
		{
			// Initialize a new level
			if (curLevel.type == BaseDatumType::Element)
			{
				iterIns = serializer.OnElement(std::static_pointer_cast<IElementDatum>(curLevel.level));
			}
			else if (curLevel.type == BaseDatumType::Object)
			{
				iterIns = serializer.OnObject(std::static_pointer_cast<IObjectDatum>(curLevel.level));
			}
			else if (curLevel.type == BaseDatumType::Dictionary)
			{
				auto pDict = std::static_pointer_cast<IDictDatum>(curLevel.level);
				bool isEmpty = pDict->IsEmpty();

				iterIns = serializer.OnDict(pDict, !isEmpty);
				if (iterIns == IterInstruction::Enter)
				{
					curLevel.LoadChildren();
				}
			}
			else if (curLevel.type == BaseDatumType::List)
			{
				auto pList = std::static_pointer_cast<IListDatum>(curLevel.level);
				bool isEmpty = pList->IsEmpty();
				iterIns = serializer.OnList(pList, !isEmpty);
				if (iterIns == IterInstruction::Enter)
				{
					curLevel.LoadChildren();
				}
			}
		}

		if (iterIns == IterInstruction::Break)
		{
			return false;
		}

		if (curLevel.HasNextChild())
		{
			// Get the next child and possibly push
			// NextChild() updates the index
			const DFSLevel::Child& nextChild = curLevel.NextChild();
			bool shouldPush{ true };
			if (curLevel.type == BaseDatumType::Dictionary)
			{
				IterInstruction keyIterIns = serializer.OnDictKey(nextChild.key.c_str());
				if (keyIterIns == IterInstruction::Break)
				{
					return false;
				}
				shouldPush = keyIterIns == IterInstruction::Enter;
			}

			if (shouldPush)
			{
				dfsStack.emplace_back(nextChild.value);
			}  
			// otherwise just move on to the next child
		}
		else
		{
			// Pop!
			if (!curLevel.children.empty())
			{
				serializer.EndCompound();
			}
			dfsStack.pop_back();
		}
	}
	return true;
}