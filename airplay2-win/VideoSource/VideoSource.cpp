/**
* John Bradley (jrb@turrettech.com)
*/
#if defined (WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <stdlib.h>
//#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

//#include <ao/ao.h>

#include "dnssd.h"
#include "airplay.h"
#include "raop.h"

#include "VideoSource.h"
#include "SDL.h"
#include "SDL_thread.h"

#if (_MSC_VER >= 1900) 

extern "C" { FILE __iob_func[3] = { *stdin,*stdout,*stderr }; }
#pragma  comment(lib, "legacy_stdio_definitions.lib") 

#endif

#if 0
extern "C" bool LoadImageFromMemory(const BYTE *buffer, unsigned int size, const char *mime, unsigned int maxwidth, unsigned int maxheight, ImageInfo *info);
extern "C" bool ReleaseImage(ImageInfo *info);
#endif

static double airtunes_audio_timestamp = -1.0;
static double airtunes_audio_clock()
{

	return airtunes_audio_timestamp;

}

static void
photo_cb(char* data, int datalen)
{
	char _template[512];
	int written = 0;
	int fd, ret;

	printf("Got photo with data length: %d\n", datalen);

	memset(_template, 0, sizeof(_template));
	strcpy(_template, "/tmp/tmpXXXXXX.JPG");
	//fd = mkstemps(_template, 4);

	//while (written < datalen) {
	//	ret = write(fd, data + written, datalen - written);
	//	if (ret <= 0) break;
	//	written += ret;
	//}
	//if (written == datalen) {
	//	printf("Wrote to file %s\n", _template);
	//}
	//close(fd);
}

static void
play_cb()
{
}

static void
stop_cb()
{
}

static void
rate_set_cb()
{
}

static void
scrub_get_cb()
{
}

static void
scrub_set_cb()
{
}

static void
playback_info_cb()
{
}

// static void*
// audio_init(void* opaque, int bits, int channels, int samplerate)
// {
// 	return NULL;
// }

static void
audio_set_volume(void* cls, void* session, float volume, const char* remoteName, const char* remoteDeviceId)
{
	printf("Setting volume to %f\n", volume);
}

static void
audio_set_metadata(void* cls, void* session, const void* buffer, int buflen, const char* remoteName, const char* remoteDeviceId)
{
	int orig = buflen;
	FILE* file = fopen("metadata.bin", "wb");
	while (buflen > 0) {
		buflen -= fwrite((char*)buffer + orig - buflen, 1, buflen, file);
	}
	fclose(file);
	printf("Metadata of length %d saved as metadata.bin\n", orig);
}

static void
audio_set_coverart(void* cls, void* session, const void* buffer, int buflen, const char* remoteName, const char* remoteDeviceId)
{
	int orig = buflen;
	FILE* file = fopen("coverart.jpg", "wb");
	while (buflen > 0) {
		buflen -= fwrite((char*)buffer + orig - buflen, 1, buflen, file);
	}
	fclose(file);
	printf("Coverart of length %d saved as coverart.jpg\n", orig);
}

static void
audio_process_ap(void* cls, void* session, const void* buffer, int buflen, const char* remoteName, const char* remoteDeviceId)
{
	printf("Got %d bytes of audio\n", buflen);
}

static void
audio_flush(void* cls, void* session, const char* remoteName, const char* remoteDeviceId)
{
}

static void
audio_destroy(void* cls, void* session, const char* remoteName, const char* remoteDeviceId)
{
	printf("Closing audio device\n");
}

static void
video_process(void* cls, h264_decode_struct* data, const char* remoteName, const char* remoteDeviceId)
{
	printf("Receive video data.[%ul]\n", data->pts);
}

static void
raop_log_callback(void* cls, int level, const char* msg)
{
	printf("RAOP LOG(%d): %s\n", level, msg);
}

VideoSource::VideoSource()
{
	audio_volume = 80;
	audio_quit = true;
	isMirrorPlaying = 0;

	memset(&xdw_decoder_q, 0, sizeof(struct __xdw_air_decoder_q));
	xdw_q_init(&xdw_decoder_q.audio_pkt_q, fn_q_mode_unblock);
	xdw_q_set_property(&xdw_decoder_q.audio_pkt_q, fn_q_node_max_nr, (void *)MAX_CACHED_AFRAME_NUM, NULL);
	xdw_q_init(&xdw_decoder_q.video_pkt_q, fn_q_mode_unblock);
	xdw_q_set_property(&xdw_decoder_q.video_pkt_q, fn_q_node_max_nr, (void *)MAX_CACHED_VFRAME_NUM, NULL);

	airtunes_audio_timestamp = -1.0;

	vs = nullptr;

    pixelData   = nullptr;
	mediaWidth  = 0;
	mediaHeight = 0;
	isAudioInited = 0;

	MUTEX_CREATE(textureLock);

	media_sws_context = nullptr;
	pMediaFrame    = av_frame_alloc();
	pMediaFrameYUV = av_frame_alloc();

	start_airplay();
}

VideoSource::~VideoSource()
{ 
	if (vs != nullptr)
	{
		delete vs;
		vs = nullptr;
	}

	av_free(pMediaFrame);
	av_free(pMediaFrameYUV);

	if (media_sws_context)
	{
		sws_freeContext(media_sws_context);
		media_sws_context = nullptr;
	}

	MUTEX_DESTROY(textureLock);

	stop_airplay();
}

void *lock(void *data, void **pixelData)
{
    VideoSource *_this = static_cast<VideoSource *>(data);

    *pixelData = _this->pixelData;

	MUTEX_LOCK(_this->textureLock);

    return NULL;
}

void unlock(void *data, void *id, void *const *pixelData)
{
    VideoSource *_this = static_cast<VideoSource *>(data);

	_this->pMediaFrame->data[0] = (uint8_t *)malloc(_this->mediaWidth * _this->mediaHeight);
	_this->pMediaFrame->data[1] = (uint8_t *)malloc((_this->mediaWidth / 2) * (_this->mediaHeight / 2));
	_this->pMediaFrame->data[2] = (uint8_t *)malloc((_this->mediaWidth / 2) * (_this->mediaHeight / 2));

	_this->pMediaFrame->linesize[0] = _this->mediaWidth;
	_this->pMediaFrame->linesize[1] = _this->mediaWidth >> 1;
	_this->pMediaFrame->linesize[2] = _this->mediaWidth >> 1;

	uint8_t *Ydata = _this->pMediaFrame->data[0];
	uint8_t *Udata = _this->pMediaFrame->data[1];
	uint8_t *Vdata = _this->pMediaFrame->data[2];
	uint8_t *PData = (uint8_t *)(*pixelData);

	for (int i = 0; i < _this->mediaHeight; i++)
	{
		for (int j = 0; j < _this->mediaWidth / 2; j++)
		{
			if ((i & 1) == 0)
			{
				*Ydata++ = *PData++;
				*Udata++ = *PData++;
				*Ydata++ = *PData++;
				*Vdata++ = *PData++;
			}
			else
			{
				*Ydata++ = *PData++;
				*PData++;
				*Ydata++ = *PData++;
				*PData++;
			}
		}
	}

	MUTEX_UNLOCK(_this->textureLock);

	SDL_Rect rect;

	int w = _this->mediaWidth;
	int h = _this->mediaHeight;

	if (!_this->media_sws_context)
	{
		_this->media_sws_context = sws_getContext(w, h, AV_PIX_FMT_YUVJ420P,
			_this->screen_w, _this->screen_h, AV_PIX_FMT_YUV420P, SWS_BICUBIC /*SWS_POINT*/,
			NULL, NULL, NULL);

		memset(_this->bmp->pixels[0], 0x0,    _this->bmp->w*_this->bmp->h);
		memset(_this->bmp->pixels[1], 0x80,  (_this->bmp->w*_this->bmp->h) >> 2);
		memset(_this->bmp->pixels[2], 0x80,  (_this->bmp->w*_this->bmp->h) >> 2);
	}

	SDL_LockYUVOverlay(_this->bmp);
	_this->pMediaFrameYUV->data[0] = _this->bmp->pixels[0];
	_this->pMediaFrameYUV->data[1] = _this->bmp->pixels[2];
	_this->pMediaFrameYUV->data[2] = _this->bmp->pixels[1];
	_this->pMediaFrameYUV->linesize[0] = _this->bmp->pitches[0];
	_this->pMediaFrameYUV->linesize[1] = _this->bmp->pitches[2];
	_this->pMediaFrameYUV->linesize[2] = _this->bmp->pitches[1];
	sws_scale(_this->media_sws_context, (const uint8_t* const*)_this->pMediaFrame->data, _this->pMediaFrame->linesize, 0,
		_this->mediaHeight, _this->pMediaFrameYUV->data, _this->pMediaFrameYUV->linesize);


	SDL_UnlockYUVOverlay(_this->bmp);

	free(_this->pMediaFrame->data[0]);
	free(_this->pMediaFrame->data[1]);
	free(_this->pMediaFrame->data[2]);

	rect.x = 0;
	rect.y = 0;
	rect.w = _this->screen_w;
	rect.h = _this->screen_h;

	SDL_DisplayYUVOverlay(_this->bmp, &rect);
}

void display(void *data, void *id)
{
}

int VideoState::queue_picture(AVFrame *pFrame)
{
	SDL_Rect rect;

	if (this->sws_context == NULL || width != pFrame->width || height != pFrame->height)
	{
		memset(((VideoSource *)this->vsource)->bmp->pixels[0], 0x0, ((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h);
		memset(((VideoSource *)this->vsource)->bmp->pixels[1], 0x80, (((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h)>>2);
		memset(((VideoSource *)this->vsource)->bmp->pixels[2], 0x80, (((VideoSource *)this->vsource)->bmp->w*((VideoSource *)this->vsource)->bmp->h)>>2);

		if (this->sws_context)
		{
			sws_freeContext(this->sws_context);
			this->sws_context = NULL;
		}

		int w = pFrame->width;
		int h = pFrame->height;

		this->sws_context = sws_getContext(w, h, (AVPixelFormat)pFrame->format,
			w, h, AV_PIX_FMT_YUV420P, SWS_BICUBIC /*SWS_POINT*/,
			NULL, NULL, NULL);
	}

	width = pFrame->width;
	height = pFrame->height;

	SDL_LockYUVOverlay(((VideoSource *)this->vsource)->bmp);
	this->pFrameYUV->data[0] = ((VideoSource *)this->vsource)->bmp->pixels[0] + ((((VideoSource *)this->vsource)->screen_w - width) >> 1);
	this->pFrameYUV->data[1] = ((VideoSource *)this->vsource)->bmp->pixels[2] + ((((VideoSource *)this->vsource)->screen_w - width) >> 2);
	this->pFrameYUV->data[2] = ((VideoSource *)this->vsource)->bmp->pixels[1] + ((((VideoSource *)this->vsource)->screen_w - width) >> 2);
	this->pFrameYUV->linesize[0] = ((VideoSource *)this->vsource)->bmp->pitches[0];
	this->pFrameYUV->linesize[1] = ((VideoSource *)this->vsource)->bmp->pitches[2];
	this->pFrameYUV->linesize[2] = ((VideoSource *)this->vsource)->bmp->pitches[1];
	sws_scale(this->sws_context, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
		pFrame->height, this->pFrameYUV->data, this->pFrameYUV->linesize);

	SDL_UnlockYUVOverlay(((VideoSource *)this->vsource)->bmp);

	rect.x = 0;
	rect.y = 0;
	rect.w = ((VideoSource *)this->vsource)->screen_w;
	rect.h = ((VideoSource *)this->vsource)->screen_h;

	SDL_DisplayYUVOverlay(((VideoSource *)this->vsource)->bmp, &rect);

	return 0;
}

void VideoState::video_thread_loop(VideoState *self)
{
	AVPacket pkt1, *packet = &pkt1;
	int frameFinished;
	AVFrame *pFrame;

	pFrame				= av_frame_alloc();
	self->pFrameYUV		= av_frame_alloc();

	while (!self->video_quit)
	{
		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *pkt_qnode;

		struct xdw_q_head *video_pkt_q = &(self->xdw_decoder_q_video->video_pkt_q);
		ptr = xdw_q_pop(video_pkt_q);
		if (ptr == NULL)
		{
			continue;
		}

		pkt_qnode = (xdw_pkt_video_qnode_t *)list_entry(ptr, xdw_pkt_video_qnode_t, list);
		if (pkt_qnode == NULL)
		{
			continue;
		}

		av_new_packet(packet, pkt_qnode->VideoBuffer->size);
		memcpy(packet->data, pkt_qnode->VideoBuffer->data, pkt_qnode->VideoBuffer->size);

		free((void *)(pkt_qnode->VideoBuffer->data));
		free((void *)(pkt_qnode->VideoBuffer));
		free((void *)pkt_qnode);

		// avcodec_decode_video2(self->codecCtx, pFrame, &frameFinished, packet);
		int ret = avcodec_send_packet(self->codecCtx, packet);
		frameFinished = avcodec_receive_frame(self->codecCtx, pFrame);

		av_packet_unref(packet);

		// Did we get a video frame?
		if (frameFinished == 0)
		{
			int ret =0;
			ret  = self->queue_picture(pFrame);
			if (ret < 0)
				break;
		}
	}

	av_free(pFrame);
	av_free(self->pFrameYUV);
}

int VideoState::init(xdw_air_decoder_q *xdw_decoder_q, const void *privatedata, int privatedatalen)
{
	xdw_decoder_q_video = xdw_decoder_q;

	MUTEX_CREATE(pictq_mutex);
	EVENT_CREATE(pictq_cond);

	this->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	this->codecCtx = avcodec_alloc_context3(codec);

	this->codecCtx->extradata = (uint8_t *)av_malloc(privatedatalen);
	this->codecCtx->extradata_size = privatedatalen;
	memcpy(this->codecCtx->extradata, privatedata, privatedatalen);
	this->codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	int res = avcodec_open2(this->codecCtx, this->codec, NULL);
	if (res < 0)
	{
		printf("Failed to initialize decoder\n");
	}

	this->video_quit = false;

	THREAD_CREATE(this->video_thread, (LPTHREAD_START_ROUTINE)video_thread_loop, this);
	this->audioClock = airtunes_audio_clock;

	return 1;
}

void VideoState::deinit()
{
	this->video_quit = true;
	EVENT_POST(this->pictq_cond);
	THREAD_JOIN(this->video_thread);

	if (sws_context)
	{
		sws_freeContext(sws_context);
		sws_context = NULL;
	}

	av_free(this->codecCtx->extradata);
	avcodec_close(this->codecCtx);
	av_free(this->codecCtx);

	MUTEX_DESTROY(this->pictq_mutex);
	EVENT_DESTORY(this->pictq_cond);
}

void VideoSource::AirPlayOutputFunctions::sdl_audio_callback(void *cls, uint8_t *stream, int len)
{
	if (!((VideoSource *)cls)->audio_quit)
	{
		while (len > 0)
		{
			struct xdw_list_head *ptr;
			xdw_pkt_audio_qnode_t *pkt_qnode;

			struct xdw_q_head *audio_pkt_q = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
			ptr = xdw_q_pop(audio_pkt_q);
			if (ptr == NULL)
			{
				memset(stream, 0, len);
				break;
			}

			pkt_qnode = (xdw_pkt_audio_qnode_t *)list_entry(ptr, xdw_pkt_audio_qnode_t, list);
			if (pkt_qnode == NULL)
			{
				memset(stream, 0, len);
				break;
			}

			SDL_MixAudio(stream, (unsigned char *)pkt_qnode->abuffer, pkt_qnode->len, ((VideoSource *)cls)->audio_volume);

			len -= pkt_qnode->len;
			stream += pkt_qnode->len;

			free((void *)(pkt_qnode->abuffer));
			free((void *)pkt_qnode);
		}
	}
	else
	{
		memset(stream, 0, len);
	}
}

void VideoSource::AirPlayOutputFunctions::audio_init(void *cls, int bits, int channels, int samplerate, int isaudio)
{
	struct xdw_q_head *q_head;
	q_head = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
	while (!xdw_q_is_empty(q_head))
	{
		struct xdw_list_head *ptr;
		xdw_pkt_audio_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);

		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_audio_qnode_t, list);
		free(frm_node->abuffer);
		free(frm_node);
	}

	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = samplerate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = 2;
	wanted_spec.silence = 0;
	if (isaudio)
		wanted_spec.samples = 1408;
	else
		wanted_spec.samples = 1920;
	wanted_spec.callback = AirPlayOutputFunctions::sdl_audio_callback;
	wanted_spec.userdata = cls;

	if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
	{
		printf("can't open audio.\n");
		return;
	}

	SDL_PauseAudio(0);
	((VideoSource *)cls)->audio_quit = false;
}

void VideoSource::AirPlayOutputFunctions::audio_process(void* cls, pcm_data_struct* data, const char* remoteName, const char* remoteDeviceId)
{
	if (((VideoSource*)cls)->isAudioInited == 0) {
		audio_init(cls, data->bits_per_sample, data->channels, data->sample_rate, 0);
		((VideoSource*)cls)->isAudioInited = 1;
	}
	airtunes_audio_timestamp = data->pts;

	if (data->data_len > 0)
	{
		unsigned char *mbuffer = (unsigned char *)malloc(data->data_len);
		xdw_pkt_audio_qnode_t *frm_node = (xdw_pkt_audio_qnode_t *)malloc(sizeof(xdw_pkt_audio_qnode_t));

		memcpy(mbuffer, data->data, data->data_len);
		frm_node->abuffer = mbuffer;
		frm_node->len = data->data_len;
		xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q));
	}
}

void VideoSource::AirPlayOutputFunctions::audio_destory(void *cls)
{
	struct xdw_q_head *q_head;
	q_head = &(((VideoSource *)cls)->xdw_decoder_q.audio_pkt_q);
	while (!xdw_q_is_empty(q_head))
	{
		struct xdw_list_head *ptr;
		xdw_pkt_audio_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);

		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_audio_qnode_t, list);
		free(frm_node->abuffer);
		free(frm_node);
	}

	if (((VideoSource *)cls)->audio_quit == false)
	{
		((VideoSource *)cls)->audio_quit = true;
		SDL_CloseAudio();
	}

	airtunes_audio_timestamp = -1.0;
}


void VideoSource::AirPlayOutputFunctions::audio_setvolume(void *cls, int volume)
{
	((VideoSource *)cls)->audio_volume = volume;

}
void VideoSource::AirPlayOutputFunctions::audio_setmetadata(void *cls, const void *buffer, int buflen)
{

}
void VideoSource::AirPlayOutputFunctions::audio_setcoverart(void *cls, const void *buffer, int buflen)
{

}

void VideoSource::AirPlayOutputFunctions::audio_flush(void *cls)
{

}



void VideoSource::AirPlayOutputFunctions::mirroring_play(void *cls, int width, int height, const void *buffer, int buflen, int payloadtype, double timestamp)
{
	struct xdw_q_head *q_head;

	q_head = &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q);
	while (!xdw_q_is_empty(q_head))
	{
		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);

		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_video_qnode_t, list);
		free(frm_node->VideoBuffer->data);
		free(frm_node->VideoBuffer);
		free(frm_node);
	}


	((VideoSource *)cls)->vs = new VideoState;
	((VideoSource *)cls)->vs->vsource = (VideoSource *)cls;
	((VideoSource *)cls)->vs->init(&(((VideoSource *)cls)->xdw_decoder_q), buffer, buflen);
}

void VideoSource::AirPlayOutputFunctions::mirroring_process(void* cls, h264_decode_struct* h264data, const char* remoteName, const char* remoteDeviceId)
{
	if (((VideoSource*)cls)->isMirrorPlaying != 1) {
		mirroring_play(cls, 1080, 1920, h264data->data, h264data->data_len, h264data->frame_type, h264data->nTimeStamp);
		((VideoSource*)cls)->isMirrorPlaying = 1;
	}

	if (h264data->data_len > 0)
	{
		if (h264data->frame_type == 0)
		{
			int spscnt;
			int spsnalsize;
			int ppscnt;
			int ppsnalsize;

			unsigned    char *head = (unsigned  char *)h264data->data;

			xdw_pkt_video_qnode_t *frm_node = (xdw_pkt_video_qnode_t *)malloc(sizeof(xdw_pkt_video_qnode_t));

			xdw_video_frm_t *pkt = (xdw_video_frm_t *)malloc(sizeof(xdw_video_frm_t));
			memset(pkt, 0, sizeof(xdw_video_frm_t));

			spscnt = head[5] & 0x1f;
			spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
			ppscnt = head[8 + spsnalsize];
			ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);

			pkt->data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);

			pkt->data[0] = 0;
			pkt->data[1] = 0;
			pkt->data[2] = 0;
			pkt->data[3] = 1;

			memcpy(pkt->data + 4, head + 8, spsnalsize);

			pkt->data[4 + spsnalsize] = 0;
			pkt->data[5 + spsnalsize] = 0;
			pkt->data[6 + spsnalsize] = 0;
			pkt->data[7 + spsnalsize] = 1;

			memcpy(pkt->data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);

			pkt->size = 4 + spsnalsize + 4 + ppsnalsize;

			frm_node->VideoBuffer = pkt;

			xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q));
		}
		else if (h264data->frame_type == 1)
		{
			int		    rLen;
			unsigned    char *data;

			xdw_pkt_video_qnode_t *frm_node = (xdw_pkt_video_qnode_t *)malloc(sizeof(xdw_pkt_video_qnode_t));

			xdw_video_frm_t *pkt = (xdw_video_frm_t *)malloc(sizeof(xdw_video_frm_t));
			memset(pkt, 0, sizeof(xdw_video_frm_t));

			pkt->data = (unsigned char *)malloc(h264data->data_len);
			memcpy(pkt->data, h264data->data, h264data->data_len);
			pkt->size = h264data->data_len;

			frm_node->VideoBuffer = pkt;

			xdw_q_push(&frm_node->list, &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q));
		}
	}
}

void VideoSource::AirPlayOutputFunctions::mirroring_stop(void *cls)
{
	if (((VideoSource *)cls)->vs != nullptr)
	{
		delete ((VideoSource *)cls)->vs;
		((VideoSource *)cls)->vs = nullptr;
	}

	struct xdw_q_head *q_head;

	q_head = &(((VideoSource *)cls)->xdw_decoder_q.video_pkt_q);
	while (!xdw_q_is_empty(q_head))
	{
		struct xdw_list_head *ptr;
		xdw_pkt_video_qnode_t *frm_node;
		ptr = xdw_q_pop(q_head);

		if (!ptr)
			break; // error
		frm_node = list_entry(ptr, xdw_pkt_video_qnode_t, list);
		free(frm_node->VideoBuffer->data);
		free(frm_node->VideoBuffer);
		free(frm_node);
	}
}

void VideoSource::start_airplay()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return ;
	}

	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	screen_w = 1920;// this->codecCtx->width;
	screen_h = 1080;// this->codecCtx->height;
	// 0x115
	screen = SDL_SetVideoMode(screen_w, screen_h, 0, SDL_SWSURFACE | SDL_RESIZABLE);

	bmp = SDL_CreateYUVOverlay(screen_w, screen_h, SDL_YV12_OVERLAY, screen);

	SDL_WM_SetCaption("XinDawn AirPlay Mirroring SDK", NULL);

	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;
	SDL_DisplayYUVOverlay(bmp, &rect);

	const char* name = "shairplay";
	unsigned short raop_port = 5000;
	unsigned short airplay_port = 7000;
	const char hwaddr[] = { 0x48, 0x5d, 0x60, 0x7c, 0xee, 0x22 };
	char* pemstr = NULL;

	dnssd_t* dnssd;
	airplay_t* airplay;
	airplay_callbacks_t ap_cbs;
	memset(&ap_cbs, 0, sizeof(airplay_callbacks_t));
	raop_t* raop;
	raop_callbacks_t raop_cbs;
	memset(&raop_cbs, 0, sizeof(raop_callbacks_t));
	raop_cbs.cls = this;

// 	ap_cbs.audio_init = audio_init;
// 	ap_cbs.audio_process = audio_process_ap;
// 	ap_cbs.audio_flush = audio_flush;
// 	ap_cbs.audio_destroy = audio_destroy;

	// raop_cbs.audio_init = audio_init;
	raop_cbs.audio_set_volume = audio_set_volume;
	raop_cbs.audio_set_metadata = audio_set_metadata;
	raop_cbs.audio_set_coverart = audio_set_coverart;
	raop_cbs.audio_process = AirPlayOutputFunctions::audio_process;
	raop_cbs.audio_flush = audio_flush;
	// raop_cbs.audio_destroy = audio_destroy;
	raop_cbs.video_process = AirPlayOutputFunctions::mirroring_process;

	int error = 0;
	airplay = airplay_init(10, &ap_cbs, pemstr, &error);
	airplay_start(airplay, &airplay_port, hwaddr, sizeof(hwaddr), NULL);

	raop = raop_init(10, &raop_cbs);

	raop_set_log_level(raop, RAOP_LOG_DEBUG);
	raop_set_log_callback(raop, &raop_log_callback, NULL);
	raop_start(raop, &raop_port);
	raop_set_port(raop, raop_port);

	dnssd = dnssd_init(&error);
	dnssd_register_raop(dnssd, name, raop_port, hwaddr, sizeof(hwaddr), 0);
	dnssd_register_airplay(dnssd, name, airplay_port, hwaddr, sizeof(hwaddr));

	printf("Startup complete... Kill with Ctrl+C\n");

	int running = 1;
	while (running != 0) {
#ifndef WIN32
		sleep(1);
#else
		Sleep(1000);
#endif
	}

	raop_stop(raop);
	raop_destroy(raop);

	airplay_stop(airplay);

	dnssd_unregister_airplay(dnssd);
	dnssd_unregister_raop(dnssd);
	dnssd_destroy(dnssd);
}

void VideoSource::stop_airplay()
{
	SDL_Quit();
}

static int running;

int main(int argc, char **argv)
{
	VideoSource *vs = NULL;

	vs =  new VideoSource();


	running = 1;
	while (running)
	{
		Sleep(1000);
	}
	
	if (vs)
	{
		delete vs;
		vs = NULL;
	}

	return 0;
}