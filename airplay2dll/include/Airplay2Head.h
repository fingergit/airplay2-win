#pragma once

#ifdef AIRPLAY2_EXPORTS
#define AIRPLAY2_API __declspec(dllexport)
#else
#define AIRPLAY2_API __declspec(dllimport)
#pragma comment(lib, "airplay2dll.lib")
#endif

#include "Airplay2Def.h"

class IAirServerCallback {
public:
	virtual void connected() = 0;
	virtual void disconnected() = 0;
	virtual void outputAudio(SFgAudioFrame* data) = 0;
	virtual void outputVideo(SFgVideoFrame* data) = 0;

	virtual void videoPlay(char* url, double volume, double startPos) = 0;
	virtual void videoGetPlayInfo(double* duration, double* position, double* rate) = 0;

	virtual void log(int level, const char* msg) = 0;
};

AIRPLAY2_API void* fgServerStart(const char serverName[AIRPLAY_NAME_LEN], IAirServerCallback* callback);
AIRPLAY2_API void fgServerStop(void* handle);

