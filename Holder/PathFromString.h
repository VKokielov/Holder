#pragma once

#include <string>
#include <vector>
#include <memory>

namespace holder::lib
{

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