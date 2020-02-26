#include "CAirServerCallback.h"
#include <stdio.h>

#define min(a,b)            (((a) < (b)) ? (a) : (b))

CAirServerCallback::CAirServerCallback()
	: screen(NULL)
	, bmp(NULL)
	, m_bAudioInited(false)
	, m_bDumpAudio(true)
	, m_fileWav(NULL)
{
	m_mutexAudio = CreateMutex(NULL, FALSE, NULL);
}

CAirServerCallback::~CAirServerCallback()
{
	CloseHandle(m_mutexAudio);
}

void CAirServerCallback::init(int width, int height)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return;
	}

	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
	// 0x115
	screen = SDL_SetVideoMode(width, height, 0, SDL_SWSURFACE | SDL_RESIZABLE);

	bmp = SDL_CreateYUVOverlay(width, height, SDL_IYUV_OVERLAY, screen);

	SDL_WM_SetCaption("AirPlay Demo", NULL);

	rect.x = 0;
	rect.y = 0;
	rect.w = width;
	rect.h = height;
	SDL_DisplayYUVOverlay(bmp, &rect);
}

void CAirServerCallback::unInit()
{
	if (NULL != screen) {
		SDL_FreeSurface(screen);
		screen = NULL;
	}
	if (NULL != bmp) {
		SDL_FreeYUVOverlay(bmp);
		bmp = NULL;
	}
	rect.w = 0;
	rect.h = 0;

	unInitAudio();

	SDL_Quit();
}

void CAirServerCallback::initAudio(SFgAudioFrame* data) {
	if ((data->sampleRate != m_sAudioFmt.sampleRate || data->channels != m_sAudioFmt.channels)) {
		unInitAudio();
	}
	if (!m_bAudioInited) {
		SDL_AudioSpec wanted_spec, obtained_spec;
		wanted_spec.freq = data->sampleRate;
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.channels = data->channels;
		wanted_spec.silence = 0;
		wanted_spec.samples = 1920;
		wanted_spec.callback = CAirServerCallback::sdlAudioCallback;
		wanted_spec.userdata = this;

		if (SDL_OpenAudio(&wanted_spec, &obtained_spec) < 0)
		{
			printf("can't open audio.\n");
			return;
		}

		SDL_PauseAudio(1);

		m_sAudioFmt.bitsPerSample = data->bitsPerSample;
		m_sAudioFmt.channels = data->channels;
		m_sAudioFmt.sampleRate = data->sampleRate;
		m_bAudioInited = true;

		if (m_bDumpAudio) {
			m_fileWav = fopen("demo-audio.wav", "wb");
		}
	}
	if (m_queueAudio.size() > 5) {
		SDL_PauseAudio(0);
	}
	//((VideoSource*)cls)->audio_quit = false;
}

void CAirServerCallback::unInitAudio()
{
	SDL_CloseAudio();
	m_bAudioInited = false;
	memset(&m_sAudioFmt, 0, sizeof(m_sAudioFmt));

	WaitForSingleObject(m_mutexAudio, INFINITE);
	while (!m_queueAudio.empty())
	{
		SDemoAudioFrame* pAudioFrame = m_queueAudio.front();
		delete[] pAudioFrame->data;
		delete pAudioFrame;
		m_queueAudio.pop();
	}
	ReleaseMutex(m_mutexAudio);

	if (m_fileWav != NULL) {
		fclose(m_fileWav);
		m_fileWav = NULL;
	}
}

void CAirServerCallback::sdlAudioCallback(void* userdata, Uint8* stream, int len)
{
	CAirServerCallback* pThis = (CAirServerCallback*)userdata;
	int needLen = len;
	int streamPos = 0;
	memset(stream, 0, len);
	WaitForSingleObject(pThis->m_mutexAudio, INFINITE);
	while (!pThis->m_queueAudio.empty())
	{
		SDemoAudioFrame* pAudioFrame = pThis->m_queueAudio.front();
		int pos = pAudioFrame->dataTotal - pAudioFrame->dataLeft;
		int readLen = min(pAudioFrame->dataLeft, needLen);

		//SDL_MixAudio(stream + streamPos, pAudioFrame->data + pos, readLen, 100);
		memcpy(stream + streamPos, pAudioFrame->data + pos, readLen);

		pAudioFrame->dataLeft -= readLen;
		needLen -= readLen;
		streamPos += readLen;
		if (pAudioFrame->dataLeft <= 0) {
			pThis->m_queueAudio.pop();
			delete[] pAudioFrame->data;
			delete pAudioFrame;
		}
		if (needLen <= 0) {
			break;
		}
	}
	ReleaseMutex(pThis->m_mutexAudio);
}

void CAirServerCallback::connected() {

}

void CAirServerCallback::disconnected() {
	unInit();
	unInitAudio();
}

void CAirServerCallback::outputAudio(SFgAudioFrame* data)
{
	if (data->channels == 0) {
		return;
	}

	initAudio(data);

	if (m_bDumpAudio) {
		if (m_fileWav != NULL) {
			fwrite(data->data, data->dataLen, 1, m_fileWav);
		}
	}

	SDemoAudioFrame* dataClone = new SDemoAudioFrame();
	dataClone->dataTotal = data->dataLen;
	dataClone->pts = data->pts;
	dataClone->dataLeft = dataClone->dataTotal;
	dataClone->data = new uint8_t[dataClone->dataTotal];
	memcpy(dataClone->data, data->data, dataClone->dataTotal);
	WaitForSingleObject(m_mutexAudio, INFINITE);
	m_queueAudio.push(dataClone);
	ReleaseMutex(m_mutexAudio);
}

void CAirServerCallback::outputVideo(SFgVideoFrame* data)
{
	if (data->width != rect.w || data->height != rect.h) {
		unInit();
		init(data->width, data->height);
	}
	SDL_LockYUVOverlay(bmp);

	for (size_t i = 0; i < data->height; i++)
	{
		memcpy(bmp->pixels[0] + i * bmp->pitches[0], data->data + i * data->pitch[0], min(bmp->pitches[0], data->pitch[0]));
		if (i % 2 == 0) {
			memcpy(bmp->pixels[1] + (i >> 1) * bmp->pitches[1], 
				data->data + data->dataLen[0] + (i >> 1) * data->pitch[1], min(bmp->pitches[1], data->pitch[1]));
			memcpy(bmp->pixels[2] + (i >> 1) * bmp->pitches[2],
				data->data + data->dataLen[0] + data->dataLen[1] + (i >> 1) * data->pitch[2], min(bmp->pitches[2], data->pitch[2]));
		}
	}

	SDL_UnlockYUVOverlay(bmp);

	rect.x = 0;
	rect.y = 0;
	rect.w = data->width;
	rect.h = data->height;

	SDL_DisplayYUVOverlay(bmp, &rect);
}
