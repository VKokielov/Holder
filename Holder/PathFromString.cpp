#include "PathFromString.h"
#include <cstring>

namespace impl_ns = holder::lib;

holder::lib::PathFromString::PathFromString(const char* pPath, char sep)
	:m_sep(sep)
{
	size_t pathLen = strlen(pPath);
	m_pathBuffer.reset(new char[pathLen+1]);
	memcpy(m_pathBuffer.get(), pPath,pathLen+1);

	char* pStoredPath = m_pathBuffer.get();
	const char* pStart = pStoredPath;
	while (*pStoredPath)
	{
		if (*pStoredPath == sep)
		{
			*pStoredPath = '\0';
			m_analyzedPath.push_back(pStart);
			pStart = pStoredPath + 1;
		}
		++pStoredPath;
	}

	// No need to set anything to '\0' here
	m_analyzedPath.push_back(pStart);
}