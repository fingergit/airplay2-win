#pragma once
#include "Airplay2Head.h"
#include "stream.h"
#include <queue>

#include "dnssd.h"
#include "airplay.h"
#include "raop.h"

extern "C"
{
#include <libavcodec/avcodec.h>
	//#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavutil/parseutils.h>
#include <libavutil/opt.h>
//#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

// H264 data for decoding
typedef struct SFgH264Data {
	int pts;
	int size;
	int is_key;
	int width;
	int height;
	unsigned char* data;
}SFgH264Data;

typedef std::queue<SFgH264Data*> FgH264DataQueue;



class FgAirplayServer
{
public:
	FgAirplayServer();   // standard constructor
	virtual ~FgAirplayServer();

	int start(const char serverName[AIRPLAY_NAME_LEN], 
		unsigned int raopPort, unsigned int airplayPort,
		IAirServerCallback* callback);
	void stop();
	float setScale(float fRatio);

protected:
	static void connected(void* cls, const char* remoteName, const char* remoteDeviceId);
	static void disconnected(void* cls, const char* remoteName, const char* remoteDeviceId);
// 	static void* audio_init(void* opaque, int bits, int channels, int samplerate);
	static void audio_set_volume(void* cls, void* session, float volume, const char* remoteName, const char* remoteDeviceId);
	static void audio_set_metadata(void* cls, void* session, const void* buffer, int buflen, const char* remoteName, const char* remoteDeviceId);
	static void audio_set_coverart(void* cls, void* session, const void* buffer, int buflen, const char* remoteName, const char* remoteDeviceId);
// 	static void audio_process_ap(void* cls, void* session, const void* buffer, int buflen);
	static void audio_process(void* cls, pcm_data_struct* data, const char* remoteName, const char* remoteDeviceId);
	static void audio_flush(void* cls, void* session, const char* remoteName, const char* remoteDeviceId);
	static void audio_destroy(void* cls, void* session, const char* remoteName, const char* remoteDeviceId);
	static void video_process(void* cls, h264_decode_struct* data, const char* remoteName, const char* remoteDeviceId);
	static void log_callback(void* cls, int level, const char* msg);

	static void ap_video_play(void* cls, char* url, double volume, double start_pos);
	static void ap_video_get_play_info(void* cls, double* duration, double* position, double* rate);

protected:
	int initFFmpeg(const void* privatedata, int privatedatalen);
	void unInitFFmpeg();
	int decodeH264Data(SFgH264Data* data, const char* remoteName, const char* remoteDeviceId);
	int scaleH264Data(SFgVideoFrame* ppFrame);

protected:
	FgH264DataQueue			m_h264Queue;
	IAirServerCallback*		m_pCallback;

	dnssd_t*				m_pDnsSd;
	airplay_t*				m_pAirplay;
	raop_t*					m_pRaop;

	airplay_callbacks_t		m_stAirplayCB;
	raop_callbacks_t		m_stRaopCB;

	volatile bool			m_bVideoQuit;
	AVCodec*				m_pCodec;
	AVCodecContext*			m_pCodecCtx;
	SwsContext*				m_pSwsCtx;
	bool					m_bCodecOpened;

	void*					m_mutexAudio;
	void*					m_mutexVideo;

	SFgVideoFrame			m_sVideoFrameOri;
	SFgVideoFrame			m_sVideoFrameScale;
	float					m_fScaleRatio;
};

