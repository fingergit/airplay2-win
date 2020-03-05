#pragma once
#include <Windows.h>

class CAutoLock
{
public:
	CAutoLock(HANDLE mutex, const char* name = NULL);
	~CAutoLock();

private:
	HANDLE m_mutex;
};

