#pragma once
#include <Windows.h>
#include "CSDLPlayer.h"


class CAirServerCallback : public IAirServerCallback
{
public:
	CAirServerCallback();
	virtual ~CAirServerCallback();

public:
	void setPlayer(CSDLPlayer* pPlayer);

public:
	virtual void connected(const char* remoteName, const char* remoteDeviceId);
	virtual void disconnected(const char* remoteName, const char* remoteDeviceId);
	virtual void outputAudio(SFgAudioFrame* data, const char* remoteName, const char* remoteDeviceId);
	virtual void outputVideo(SFgVideoFrame* data, const char* remoteName, const char* remoteDeviceId);

	virtual void videoPlay(char* url, double volume, double startPos);
	virtual void videoGetPlayInfo(double* duration, double* position, double* rate);

	virtual void log(int level, const char* msg);

protected:
	CSDLPlayer* m_pPlayer;
	char m_chRemoteDeviceId[128];
};

