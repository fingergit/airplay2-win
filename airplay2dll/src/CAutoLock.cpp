#include "CAutoLock.h"
#include <stdio.h>

CAutoLock::CAutoLock(HANDLE mutex, const char* name)
	: m_mutex(mutex)
{
	int idx = 0;
	while (true)
	{
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_mutex, 10000))
		{
			break;
		}
		else 
		{
			printf("Waiting for mutex timeout: %s [%d] times\n", name, idx + 1);
			idx++;
		}
	}
}

CAutoLock::~CAutoLock()
{
	ReleaseMutex(m_mutex);
}
