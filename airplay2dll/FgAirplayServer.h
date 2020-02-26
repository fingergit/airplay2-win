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

	int start(const char serverName[AIRPLAY_NAME_LEN], IAirServerCallback* callback);
	void stop();

protected:
	static void connected(void* cls);
	static void disconnected(void* cls);
	static void* audio_init(void* opaque, int bits, int channels, int samplerate);
	static void audio_set_volume(void* cls, void* session, float volume);
	static void audio_set_metadata(void* cls, void* session, const void* buffer, int buflen);
	static void audio_set_coverart(void* cls, void* session, const void* buffer, int buflen);
	static void audio_process_ap(void* cls, void* session, const void* buffer, int buflen);
	static void audio_process(void* cls, pcm_data_struct* data);
	static void audio_flush(void* cls, void* session);
	static void audio_destroy(void* cls, void* session);
	static void video_process(void* cls, h264_decode_struct* data);
	static void raop_log_callback(void* cls, int level, const char* msg);

protected:
	int initFFmpeg(const void* privatedata, int privatedatalen);
	int decodeH264Data(SFgH264Data* data);

protected:
	FgH264DataQueue m_h264Queue;
	IAirServerCallback* m_callback;

	dnssd_t* dnssd;
	airplay_t* airplay;
	raop_t* raop;

	airplay_callbacks_t ap_cbs;
	raop_callbacks_t raop_cbs;

	volatile bool video_quit;
	AVCodec* codec;
	AVCodecContext* codecCtx;
	bool isCodecOpened;


};

