#pragma once
#include <queue>
#include "Airplay2Head.h"

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


class FgAirplayChannel
{
public:
	FgAirplayChannel(IAirServerCallback* pCallback);
	~FgAirplayChannel();

public:
	long addRef();
	long release();

	int initFFmpeg(const void* privatedata, int privatedatalen);
	void unInitFFmpeg();
	float setScale(float fRatio);
	int decodeH264Data(SFgH264Data* data, const char* remoteName, const char* remoteDeviceId);
	int scaleH264Data(SFgVideoFrame* ppFrame);

protected:
	long m_nRef;

	FgH264DataQueue			m_h264Queue;
	IAirServerCallback*		m_pCallback;

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

