#include "CAirServerCallback.h"
#include <stdio.h>

#define min(a,b)            (((a) < (b)) ? (a) : (b))

CAirServerCallback::CAirServerCallback()
	: m_pPlayer(NULL)
{
}

CAirServerCallback::~CAirServerCallback()
{
}

void CAirServerCallback::setPlayer(CSDLPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

void CAirServerCallback::connected() {

}

void CAirServerCallback::disconnected() {
}

void CAirServerCallback::outputAudio(SFgAudioFrame* data)
{
	if (m_pPlayer)
	{
		m_pPlayer->outputAudio(data);
	}
}

void CAirServerCallback::outputVideo(SFgVideoFrame* data)
{
	if (m_pPlayer)
	{
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
