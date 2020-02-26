#pragma once
#include <Windows.h>
#include "Airplay2Head.h"
#include <queue>

#include "SDL.h"
#include "SDL_thread.h"
#undef main 

typedef struct SDemoAudioFrame {
	unsigned long long pts;
	unsigned int dataTotal;
	unsigned int dataLeft;
	unsigned char* data;
} SDemoAudioFrame;


typedef std::queue<SDemoAudioFrame*> SDemoAudioFrameQueue;

class CAirServerCallback : public IAirServerCallback
{
public:
	CAirServerCallback();
	virtual ~CAirServerCallback();

public:
	void init(int width, int height);
	void unInit();
	void initAudio(SFgAudioFrame* data);
	void unInitAudio();
	static void sdlAudioCallback(void* userdata, Uint8* stream, int len);

public:
	virtual void connected();
	virtual void disconnected();
	virtual void outputAudio(SFgAudioFrame* data);
	virtual void outputVideo(SFgVideoFrame* data);

protected:
	SDL_Surface* screen;
	//SDL_VideoInfo* vi;
	SDL_Overlay* bmp;
	SDL_Rect rect;

	SFgAudioFrame m_sAudioFmt;
	bool m_bAudioInited;
	SDemoAudioFrameQueue m_queueAudio;
	HANDLE m_mutexAudio;

	bool m_bDumpAudio;
	FILE* m_fileWav;

};

