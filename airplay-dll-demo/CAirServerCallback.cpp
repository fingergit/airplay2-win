#include "CAirServerCallback.h"
#include <stdio.h>
#include "FgUtf8Utils.h"
#include <locale.h>

#define min(a,b)            (((a) < (b)) ? (a) : (b))

CAirServerCallback::CAirServerCallback()
	: m_pPlayer(NULL)
{
	memset(m_chRemoteDeviceId, 0, 128);
}

CAirServerCallback::~CAirServerCallback()
{
}

void CAirServerCallback::setPlayer(CSDLPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

void CAirServerCallback::connected(const char* remoteName, const char* remoteDeviceId) {
	if (remoteDeviceId != NULL) {
		strncpy(m_chRemoteDeviceId, remoteDeviceId, 128);
	}

	setlocale(LC_CTYPE, "");
	std::wstring name = CFgUtf8Utils::UTF8_To_UTF16(remoteName);
	wprintf(L"Client Name: %s\n", name.c_str());
}

void CAirServerCallback::disconnected(const char* remoteName, const char* remoteDeviceId) {
	memset(m_chRemoteDeviceId, 0, 128);
}

void CAirServerCallback::outputAudio(SFgAudioFrame* data, const char* remoteName, const char* remoteDeviceId)
{
	if (m_pPlayer)
	{
		if (m_chRemoteDeviceId[0] == '\0' && remoteDeviceId != NULL)
		{
			strncpy(m_chRemoteDeviceId, remoteDeviceId, 128);
		}
		if (0 != strcmp(m_chRemoteDeviceId, remoteDeviceId))
		{
			return;
		}
		m_pPlayer->outputAudio(data);
	}
}

void CAirServerCallback::outputVideo(SFgVideoFrame* data, const char* remoteName, const char* remoteDeviceId)
{
	if (m_pPlayer)
	{
		if (m_chRemoteDeviceId[0] == '\0' && remoteDeviceId != NULL)
		{
			strncpy(m_chRemoteDeviceId, remoteDeviceId, 128);
		}
		if (0 != strcmp(m_chRemoteDeviceId, remoteDeviceId))
		{
			return;
		}
		m_pPlayer->outputVideo(data);
	}
}

void CAirServerCallback::videoPlay(char* url, double volume, double startPos)
{
	printf("Play: %s", url);
}

double dbDuration = 10000;
double dbPosition = 0;
void CAirServerCallback::videoGetPlayInfo(double* duration, double* position, double* rate)
{
	*duration = 1000;
	*position = dbPosition;
	*rate = 1.0;
	dbPosition += 1;
}

void CAirServerCallback::log(int level, const char* msg)
{
#ifdef _DEBUG
	OutputDebugStringA(msg);
	OutputDebugStringA("\n");
#else
	printf("%s\n", msg);
#endif
}
