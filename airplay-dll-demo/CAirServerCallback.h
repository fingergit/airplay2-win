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
	virtual void connected();
	virtual void disconnected();
	virtual void outputAudio(SFgAudioFrame* data);
	virtual void outputVideo(SFgVideoFrame* data);

	virtual void videoPlay(char* url, double volume, double startPos);
	virtual void videoGetPlayInfo(double* duration, double* position, double* rate);

	virtual void log(int level, const char* msg);

protected:
	CSDLPlayer* m_pPlayer;
};

