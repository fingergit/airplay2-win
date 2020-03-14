/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "xdw_list.h"
#include "threads.h"
#include "SDL.h"
#include "SDL_thread.h"

#include <vector>

#define CHROMA "YUYV"


#define __STDC_CONSTANT_MACROS

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

#define MAX_CACHED_AFRAME_NUM 100
#define MAX_CACHED_VFRAME_NUM 10

typedef struct __xdw_air_decoder_q
{
	struct xdw_q_head video_pkt_q;	// raw packet q that not be decoded
	struct xdw_q_head audio_pkt_q;
}xdw_air_decoder_q;


typedef struct __xdw_pkt_audio_qnode_t
{
	unsigned char *abuffer;
	int len;
	struct xdw_list_head list;
}xdw_pkt_audio_qnode_t;

typedef struct __xdw_video_frm_t
{
	int pts;
	int size;
	int is_key;
	int width;
	int height;
	unsigned char *data;
}xdw_video_frm_t;


typedef struct _VideoPktBufferPool
{
	xdw_video_frm_t *VideoBuffer;
	struct xdw_list_head list;
}xdw_pkt_video_qnode_t;

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)
#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0
#define AUDIO_DIFF_AVG_NB 20
#define VIDEO_PICTURE_QUEUE_SIZE 1

enum
{
	AV_SYNC_AUDIO_MASTER,
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_MASTER,

	AV_SYNC_DEFAULT = AV_SYNC_EXTERNAL_MASTER
};

typedef double(* GetAudioClockHandle)(void);

struct VideoState;
struct VideoSource;

struct VideoState
{
	VideoState()
	:  sws_context(NULL), pFrameYUV(NULL)
	, video_quit(false), xdw_decoder_q_video(NULL), width(0), height(0)
	{
		// Register all formats and codecs
		// av_register_all();
	}

	~VideoState()
	{
		deinit();
	}

	int  init(xdw_air_decoder_q *xdw_decoder_q,const void *privatedata, int privatedatalen);
	void deinit();


	static void video_thread_loop(VideoState *is);
	int queue_picture(AVFrame *pFrame);

	GetAudioClockHandle audioClock;

	SwsContext*  sws_context;

	AVFrame*     pFrameYUV; // used as buffer for the frame converted from its native format to RGBA

	thread_handle_t video_thread;
	thread_handle_t refresh_thread;

	mutex_handle_t pictq_mutex;
	event_handle_t pictq_cond;

	xdw_air_decoder_q *xdw_decoder_q_video;

	volatile bool video_quit;
	AVCodec *codec;
	AVCodecContext *codecCtx;

	int width;
	int height;

	struct VideoSource *vsource;
};

class VideoSource
{
public:
    VideoSource();
    ~VideoSource();

public:
	xdw_air_decoder_q xdw_decoder_q;

	struct VideoState  *vs;
	void start_airplay();
	void stop_airplay();
	volatile bool audio_quit;

	int  audio_volume;

public:
	mutex_handle_t textureLock;

    void *pixelData;

    unsigned int mediaWidth;
    unsigned int mediaHeight;

    // should only be set inside texture lock
    bool isRendering;
    int remainingVideos;

	SwsContext*  media_sws_context;
	AVFrame*     pMediaFrame;
	AVFrame*     pMediaFrameYUV; // used as buffer for the frame converted from its native format to RGBA

public:
	//SDL
	int screen_w, screen_h;
	SDL_Surface *screen;
	SDL_VideoInfo *vi;
	SDL_Overlay *bmp;
	SDL_Rect rect;

public:
	int isMirrorPlaying;
	int isAudioInited;

public:

	class AirPlayOutputFunctions
	{
	public:
		static void audio_init(void *cls, int bits, int channels, int samplerate, int isaudio);
		static void audio_process(void* cls, pcm_data_struct* data, const char* remoteName, const char* remoteDeviceId);
		static void audio_destory(void *cls);
		static void audio_setvolume(void *cls, int volume);//1-100
		static void audio_setmetadata(void *cls, const void *buffer, int buflen);
		static void audio_setcoverart(void *cls, const void *buffer, int buflen);
		static void audio_flush(void *cls);
		static void mirroring_play(void *cls, int width, int height, const void *buffer, int buflen, int payloadtype, double timestamp);
		static void mirroring_process(void* cls, h264_decode_struct* data, const char* remoteName, const char* remoteDeviceId);
		static void mirroring_stop(void *cls);

		static void sdl_audio_callback(void *cls, uint8_t *stream, int len);
	};
};



