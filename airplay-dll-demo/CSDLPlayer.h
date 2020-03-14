#pragma once
#include <Windows.h>
#include "Airplay2Head.h"
#include <queue>
#include "SDL.h"
#include "SDL_thread.h"
#undef main 
#include "CAirServer.h"

typedef void sdlAudioCallback(void* userdata, Uint8* stream, int len);

typedef struct SDemoAudioFrame {
	unsigned long long pts;
	unsigned int dataTotal;
	unsigned int dataLeft;
	unsigned char* data;
} SDemoAudioFrame;

typedef std::queue<SDemoAudioFrame*> SDemoAudioFrameQueue;
typedef std::queue<SFgVideoFrame*> SFgVideoFrameQueue;

#define VIDEO_SIZE_CHANGED_CODE 1

class CSDLPlayer
{
public:
	CSDLPlayer();
	~CSDLPlayer();

	bool init();
	void unInit();
	void loopEvents();

	void outputVideo(SFgVideoFrame* data);
	void outputAudio(SFgAudioFrame* data);

	void initVideo(int width, int height);
	void unInitVideo();
	void initAudio(SFgAudioFrame* data);
	void unInitAudio();
	static void sdlAudioCallback(void* userdata, Uint8* stream, int len);

	SDL_Surface* m_surface;
	//SDL_VideoInfo* vi;
	SDL_Overlay* m_yuv;
	SDL_Rect m_rect;

	SFgAudioFrame m_sAudioFmt;
	bool m_bAudioInited;
	SDemoAudioFrameQueue m_queueAudio;
	HANDLE m_mutexAudio;
	HANDLE m_mutexVideo;

	SDL_Event m_evtVideoSizeChange;

	bool m_bDumpAudio;
	FILE* m_fileWav;

	CAirServer m_server;
	float m_fRatio;
};

