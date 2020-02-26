#include "pch.h"
#include "FgAirplayServer.h"
#include "Airplay2Head.h"


FgAirplayServer::FgAirplayServer()
{
	memset(&ap_cbs, 0, sizeof(airplay_callbacks_t));
	memset(&raop_cbs, 0, sizeof(raop_callbacks_t));
	raop_cbs.cls = this;

	ap_cbs.audio_init = audio_init;
	ap_cbs.audio_process = audio_process_ap;
	ap_cbs.audio_flush = audio_flush;
	ap_cbs.audio_destroy = audio_destroy;

	raop_cbs.connected = connected;
	raop_cbs.disconnected = disconnected;
	// raop_cbs.audio_init = audio_init;
	raop_cbs.audio_set_volume = audio_set_volume;
	raop_cbs.audio_set_metadata = audio_set_metadata;
	raop_cbs.audio_set_coverart = audio_set_coverart;
	raop_cbs.audio_process = audio_process;
	raop_cbs.audio_flush = audio_flush;
	// raop_cbs.audio_destroy = audio_destroy;
	raop_cbs.video_process = video_process;

	video_quit = false;
	codec = NULL;
	codecCtx = NULL;
	isCodecOpened = false;
}

FgAirplayServer::~FgAirplayServer()
{
}

int FgAirplayServer::start(const char serverName[AIRPLAY_NAME_LEN], IAirServerCallback* callback)
{
	m_callback = callback;

	unsigned short raop_port = 5000;
	unsigned short airplay_port = 7000;
	const char hwaddr[] = { 0x48, 0x5d, 0x60, 0x7c, 0xee, 0x22 };
	char* pemstr = NULL;

	int ret = 0;
	do {
		airplay = airplay_init(10, &ap_cbs, pemstr, &ret);
		if (airplay == NULL) {
			ret = -1;
			break;
		}
		ret = airplay_start(airplay, &airplay_port, hwaddr, sizeof(hwaddr), NULL);
		if (ret < 0) {
			break;
		}

		raop = raop_init(10, &raop_cbs);
		if (raop == NULL) {
			ret = -1;
			break;
		}

		raop_set_log_level(raop, RAOP_LOG_DEBUG);
		raop_set_log_callback(raop, &raop_log_callback, NULL);
		ret = raop_start(raop, &raop_port);
		if (ret < 0) {
			break;
		}
		raop_set_port(raop, raop_port);

		dnssd = dnssd_init(&ret);
		if (dnssd == NULL) {
			ret = -1;
			break;
		}
		ret = dnssd_register_raop(dnssd, serverName, raop_port, hwaddr, sizeof(hwaddr), 0);
		if (ret < 0) {
			break;
		}
		ret = dnssd_register_airplay(dnssd, serverName, airplay_port, hwaddr, sizeof(hwaddr));
		if (ret < 0) {
			break;
		}

		raop_log_info(raop, "Startup complete... Kill with Ctrl+C\n");
	} while (false);

	if (ret != 0) {
		stop();
	}

	return 0;
}

void FgAirplayServer::stop()
{
	if (raop) {
		raop_destroy(raop);
		raop = NULL;
	}

	if (airplay) {
		airplay_destroy(airplay);
		airplay = NULL;
	}

	if (dnssd) {
		dnssd_unregister_airplay(dnssd);
		dnssd_unregister_raop(dnssd);
		dnssd_destroy(dnssd);
		dnssd = NULL;
	}
}

void FgAirplayServer::connected(void* cls)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (pServer->m_callback != NULL)
	{
		pServer->m_callback->connected();
	}
}

void FgAirplayServer::disconnected(void* cls)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (pServer->m_callback != NULL)
	{
		pServer->m_callback->disconnected();
	}
}

void* FgAirplayServer::audio_init(void* opaque, int bits, int channels, int samplerate)
{
	return nullptr;
}

void FgAirplayServer::audio_set_volume(void* cls, void* session, float volume)
{
}

void FgAirplayServer::audio_set_metadata(void* cls, void* session, const void* buffer, int buflen)
{
}

void FgAirplayServer::audio_set_coverart(void* cls, void* session, const void* buffer, int buflen)
{
}

void FgAirplayServer::audio_process_ap(void* cls, void* session, const void* buffer, int buflen)
{
}

void FgAirplayServer::audio_process(void* cls, pcm_data_struct* data)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (pServer->m_callback != NULL)
	{
		SFgAudioFrame* frame = new SFgAudioFrame();
		frame->bitsPerSample = data->bits_per_sample;
		frame->channels = data->channels;
		frame->pts = data->pts;
		frame->sampleRate = data->sample_rate;
		frame->dataLen = data->data_len;
		frame->data = new uint8_t[frame->dataLen];
		memcpy(frame->data, data->data, frame->dataLen);

		pServer->m_callback->outputAudio(frame);
		delete[] frame->data;
		delete frame;
	}
}

void FgAirplayServer::audio_flush(void* cls, void* session)
{
}

void FgAirplayServer::audio_destroy(void* cls, void* session)
{
}

void FgAirplayServer::video_process(void* cls, h264_decode_struct* h264data)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (h264data->data_len <= 0)
	{
		return;
	}

	SFgH264Data* pData = new SFgH264Data();
	memset(pData, 0, sizeof(SFgH264Data));

	if (h264data->frame_type == 0)
	{
		pData->size = h264data->data_len;
		pData->data = new uint8_t[pData->size];
		pData->is_key = 1;
		memcpy(pData->data, h264data->data, h264data->data_len);
	}
	else if (h264data->frame_type == 1)
	{
		pData->size = h264data->data_len;
		pData->data = new uint8_t[pData->size];
		memcpy(pData->data, h264data->data, h264data->data_len);
	}

	pServer->decodeH264Data(pData);
	delete[] pData->data;
	delete pData;
}

void FgAirplayServer::raop_log_callback(void* cls, int level, const char* msg)
{
	printf("%s\n", msg);
}

int FgAirplayServer::initFFmpeg(const void* privatedata, int privatedatalen) {
	if (codec == NULL) {
		this->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
		this->codecCtx = avcodec_alloc_context3(codec);
	}
	if (codec == NULL) {
		return -1;
	}

	this->codecCtx->extradata = (uint8_t*)av_malloc(privatedatalen);
	this->codecCtx->extradata_size = privatedatalen;
	memcpy(this->codecCtx->extradata, privatedata, privatedatalen);
	this->codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	int res = avcodec_open2(this->codecCtx, this->codec, NULL);
	if (res < 0)
	{
		printf("Failed to initialize decoder\n");
		return -1;
	}

	isCodecOpened = true;
	this->video_quit = false;

	return 0;
}

int FgAirplayServer::decodeH264Data(SFgH264Data* data) {
	int ret = 0;
	if (!isCodecOpened && !data->is_key) {
		return 0;
	}
	if (data->is_key) {
		ret = initFFmpeg(data->data, data->size);
		if (ret < 0) {
			return ret;
		}
	}
	if (!isCodecOpened) {
		return 0;
	}

	AVPacket pkt1, * packet = &pkt1;
	int frameFinished;
	AVFrame* pFrame;

	pFrame = av_frame_alloc();

	av_new_packet(packet, data->size);
	memcpy(packet->data, data->data, data->size);

	ret = avcodec_send_packet(this->codecCtx, packet);
	frameFinished = avcodec_receive_frame(this->codecCtx, pFrame);

	av_packet_unref(packet);

	// Did we get a video frame?
	if (frameFinished == 0)
	{
		SFgVideoFrame* pVideoFrame = new SFgVideoFrame();
		pVideoFrame->width = pFrame->width;
		pVideoFrame->height = pFrame->height;
		pVideoFrame->pts = pFrame->pts;
		pVideoFrame->isKey = pFrame->key_frame;
		int ySize = pFrame->linesize[0] * pFrame->height;
		int uSize = pFrame->linesize[1] * pFrame->height >> 1;
		int vSize = pFrame->linesize[2] * pFrame->height >> 1;
		pVideoFrame->dataTotalLen = ySize + uSize + vSize;
		pVideoFrame->dataLen[0] = ySize;
		pVideoFrame->dataLen[1] = uSize;
		pVideoFrame->dataLen[2] = vSize;
		pVideoFrame->data = new uint8_t[pVideoFrame->dataTotalLen];
		memcpy(pVideoFrame->data, pFrame->data[0], ySize);
		memcpy(pVideoFrame->data + ySize, pFrame->data[1], uSize);
		memcpy(pVideoFrame->data + ySize + uSize, pFrame->data[2], vSize);
		pVideoFrame->pitch[0] = pFrame->linesize[0];
		pVideoFrame->pitch[1] = pFrame->linesize[1];
		pVideoFrame->pitch[2] = pFrame->linesize[2];

		if (m_callback != NULL)
		{
			m_callback->outputVideo(pVideoFrame);
		}
		delete[] pVideoFrame->data;
		delete pVideoFrame;
	}
	av_frame_free(&pFrame);

	return 0;
}
