#include "pch.h"
#include "FgAirplayServer.h"
#include "Airplay2Head.h"
#include <thread>
#include "CAutoLock.h"

#ifdef WIN32
#include   "iphlpapi.h"  
#pragma   comment(lib,   "iphlpapi.lib   ")  
#endif

BOOL GetMacAddress(char strMac[6]);

FgAirplayServer::FgAirplayServer()
	: m_pCallback(NULL)
	, m_pDnsSd(NULL)
	, m_pAirplay(NULL)
	, m_pRaop(NULL)
	, m_bVideoQuit(false)
	, m_pCodec(NULL)
	, m_pCodecCtx(NULL)
	, m_pSwsCtx(NULL)
	, m_bCodecOpened(false)
	, m_fScaleRatio(1.0f)
{
	memset(&m_stAirplayCB, 0, sizeof(airplay_callbacks_t));
	memset(&m_stRaopCB, 0, sizeof(raop_callbacks_t));
	m_stAirplayCB.cls = this;
	m_stRaopCB.cls = this;

// 	m_stAirplayCB.audio_init = audio_init;
// 	m_stAirplayCB.audio_process = audio_process_ap;
// 	m_stAirplayCB.audio_flush = audio_flush;
// 	m_stAirplayCB.audio_destroy = audio_destroy;
	m_stAirplayCB.video_play = ap_video_play;
	m_stAirplayCB.video_get_play_info = ap_video_get_play_info;

	m_stRaopCB.connected = connected;
	m_stRaopCB.disconnected = disconnected;
	// m_stRaopCB.audio_init = audio_init;
	m_stRaopCB.audio_set_volume = audio_set_volume;
	m_stRaopCB.audio_set_metadata = audio_set_metadata;
	m_stRaopCB.audio_set_coverart = audio_set_coverart;
	m_stRaopCB.audio_process = audio_process;
	m_stRaopCB.audio_flush = audio_flush;
	// m_stRaopCB.audio_destroy = audio_destroy;
	m_stRaopCB.video_process = video_process;

	memset(&m_sVideoFrameOri, 0, sizeof(SFgVideoFrame));
	memset(&m_sVideoFrameScale, 0, sizeof(SFgVideoFrame));

	m_mutexAudio = CreateMutex(NULL, FALSE, NULL);
	m_mutexVideo = CreateMutex(NULL, FALSE, NULL);
}

FgAirplayServer::~FgAirplayServer()
{
	m_pCallback = NULL;
	if (m_sVideoFrameOri.data)
	{
		delete[] m_sVideoFrameOri.data;
		m_sVideoFrameOri.data = NULL;
	}
	if (m_sVideoFrameScale.data)
	{
		delete[] m_sVideoFrameScale.data;
		m_sVideoFrameScale.data = NULL;
	}

	CloseHandle(m_mutexAudio);
	CloseHandle(m_mutexVideo);
}

int FgAirplayServer::start(const char serverName[AIRPLAY_NAME_LEN], 
	unsigned int raopPort, unsigned int airplayPort,
	IAirServerCallback* callback)
{
	m_pCallback = callback;

	unsigned short raop_port = raopPort;
	unsigned short airplay_port = airplayPort;
	char hwaddr[] = { 0x48, 0x5d, 0x60, 0x7c, 0xee, 0x22 };
	char* pemstr = NULL;

	int ret = 0;
	do {

		GetMacAddress(hwaddr);

		m_pAirplay = airplay_init(10, &m_stAirplayCB, pemstr, &ret);
		if (m_pAirplay == NULL) {
			ret = -1;
			break;
		}
		ret = airplay_start(m_pAirplay, &airplay_port, hwaddr, sizeof(hwaddr), NULL);
		if (ret < 0) {
			break;
		}
		airplay_set_log_level(m_pAirplay, RAOP_LOG_DEBUG);
		airplay_set_log_callback(m_pAirplay, &log_callback, this);

		m_pRaop = raop_init(10, &m_stRaopCB);
		if (m_pRaop == NULL) {
			ret = -1;
			break;
		}

		raop_set_log_level(m_pRaop, RAOP_LOG_DEBUG);
		raop_set_log_callback(m_pRaop, &log_callback, this);
		ret = raop_start(m_pRaop, &raop_port);
		if (ret < 0) {
			break;
		}
		raop_set_port(m_pRaop, raop_port);

		m_pDnsSd = dnssd_init(&ret);
		if (m_pDnsSd == NULL) {
			ret = -1;
			break;
		}
		ret = dnssd_register_raop(m_pDnsSd, serverName, raop_port, hwaddr, sizeof(hwaddr), 0);
		if (ret < 0) {
			break;
		}
		ret = dnssd_register_airplay(m_pDnsSd, serverName, airplay_port, hwaddr, sizeof(hwaddr));
		if (ret < 0) {
			break;
		}

		raop_log_info(m_pRaop, "Startup complete... Kill with Ctrl+C\n");
	} while (false);

	if (ret != 0) {
		stop();
	}

	return 0;
}

void FgAirplayServer::stop()
{
	if (m_pDnsSd) {
		dnssd_unregister_airplay(m_pDnsSd);
		dnssd_unregister_raop(m_pDnsSd);
		dnssd_destroy(m_pDnsSd);
		m_pDnsSd = NULL;
	}

	if (m_pRaop) {
		raop_destroy(m_pRaop);
// 		raop_set_log_callback(m_pRaop, &log_callback, NULL);
		m_pRaop = NULL;
	}

	if (m_pAirplay) {
		airplay_destroy(m_pAirplay);
// 		airplay_set_log_callback(m_pAirplay, &log_callback, NULL);
		m_pAirplay = NULL;
	}

	unInitFFmpeg();
	m_pCallback = NULL;
}

float FgAirplayServer::setScale(float fRatio)
{
	m_fScaleRatio = min(10, max(0.1, fRatio));
	return m_fScaleRatio;
}

void FgAirplayServer::connected(void* cls, const char* remoteName, const char* remoteDeviceId)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (!pServer)
	{
		return;
	}
	if (pServer->m_pCallback != NULL)
	{
		pServer->m_pCallback->connected(remoteName, remoteDeviceId);
	}
}

void FgAirplayServer::disconnected(void* cls, const char* remoteName, const char* remoteDeviceId)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (!pServer)
	{
		return;
	}
	if (pServer->m_pCallback != NULL)
	{
		pServer->m_pCallback->disconnected(remoteName, remoteDeviceId);
	}
}

// void* FgAirplayServer::audio_init(void* opaque, int bits, int channels, int samplerate)
// {
// 	return nullptr;
// }

void FgAirplayServer::audio_set_volume(void* cls, void* session, float volume, const char* remoteName, const char* remoteDeviceId)
{
}

void FgAirplayServer::audio_set_metadata(void* cls, void* session, const void* buffer, int buflen, const char* remoteName, const char* remoteDeviceId)
{
}

void FgAirplayServer::audio_set_coverart(void* cls, void* session, const void* buffer, int buflen, const char* remoteName, const char* remoteDeviceId)
{
}

// void FgAirplayServer::audio_process_ap(void* cls, void* session, const void* buffer, int buflen)
// {
// }

void FgAirplayServer::audio_process(void* cls, pcm_data_struct* data, const char* remoteName, const char* remoteDeviceId)
{

	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (!pServer)
	{
		return;
	}
	if (pServer->m_pCallback != NULL)
	{
		CAutoLock oLock(pServer->m_mutexAudio, "audio_process");

		SFgAudioFrame* frame = new SFgAudioFrame();
		frame->bitsPerSample = data->bits_per_sample;
		frame->channels = data->channels;
		frame->pts = data->pts;
		frame->sampleRate = data->sample_rate;
		frame->dataLen = data->data_len;
		frame->data = new uint8_t[frame->dataLen];
		memcpy(frame->data, data->data, frame->dataLen);

		pServer->m_pCallback->outputAudio(frame, remoteName, remoteDeviceId);
		delete[] frame->data;
		delete frame;
	}
}

void FgAirplayServer::audio_flush(void* cls, void* session, const char* remoteName, const char* remoteDeviceId)
{
}

void FgAirplayServer::audio_destroy(void* cls, void* session, const char* remoteName, const char* remoteDeviceId)
{
}

void FgAirplayServer::video_process(void* cls, h264_decode_struct* h264data, const char* remoteName, const char* remoteDeviceId)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (!pServer)
	{
		return;
	}
	if (h264data->data_len <= 0)
	{
		return;
	}

	CAutoLock oLock(pServer->m_mutexVideo, "video_process");

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

	pServer->decodeH264Data(pData, remoteName, remoteDeviceId);
	delete[] pData->data;
	delete pData;
}

void FgAirplayServer::ap_video_play(void* cls, char* url, double volume, double start_pos)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (!pServer)
	{
		return;
	}
	if (pServer->m_pCallback)
	{
		pServer->m_pCallback->videoPlay(url, volume, start_pos);
	}
}

void FgAirplayServer::ap_video_get_play_info(void* cls, double* duration, double* position, double* rate)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (!pServer)
	{
		return;
	}
	if (pServer->m_pCallback)
	{
		pServer->m_pCallback->videoGetPlayInfo(duration, position, rate);
	}
}

void FgAirplayServer::log_callback(void* cls, int level, const char* msg)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (!pServer) 
	{
		return;
	}
	if (pServer->m_pCallback)
	{
		pServer->m_pCallback->log(level, msg);
	}
}

int FgAirplayServer::initFFmpeg(const void* privatedata, int privatedatalen) {
	if (m_pCodec == NULL) {
		m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
		m_pCodecCtx = avcodec_alloc_context3(m_pCodec);
	}
	if (m_pCodec == NULL) {
		return -1;
	}

	m_pCodecCtx->extradata = (uint8_t*)av_malloc(privatedatalen);
	m_pCodecCtx->extradata_size = privatedatalen;
	memcpy(m_pCodecCtx->extradata, privatedata, privatedatalen);
	m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	int res = avcodec_open2(m_pCodecCtx, m_pCodec, NULL);
	if (res < 0)
	{
		printf("Failed to initialize decoder\n");
		return -1;
	}

	m_bCodecOpened = true;
	this->m_bVideoQuit = false;

	return 0;
}

void FgAirplayServer::unInitFFmpeg()
{
	if (m_pCodecCtx)
	{
		if (m_pCodecCtx->extradata) {
			av_freep(&m_pCodecCtx->extradata);
		}
		avcodec_free_context(&m_pCodecCtx);
		m_pCodecCtx = NULL;
	}
	if (m_pSwsCtx)
	{
		sws_freeContext(m_pSwsCtx);
		m_pSwsCtx = NULL;
	}
}

int FgAirplayServer::decodeH264Data(SFgH264Data* data, const char* remoteName, const char* remoteDeviceId) {
	int ret = 0;
	if (!m_bCodecOpened && !data->is_key) {
		return 0;
	}
	if (data->is_key) {
		ret = initFFmpeg(data->data, data->size);
		if (ret < 0) {
			return ret;
		}
	}
	if (!m_bCodecOpened) {
		return 0;
	}

	AVPacket pkt1, * packet = &pkt1;
	int frameFinished;
	AVFrame* pFrame;

	pFrame = av_frame_alloc();

	av_new_packet(packet, data->size);
	memcpy(packet->data, data->data, data->size);

	ret = avcodec_send_packet(this->m_pCodecCtx, packet);
	frameFinished = avcodec_receive_frame(this->m_pCodecCtx, pFrame);

	av_packet_unref(packet);

	// Did we get a video frame?
	if (frameFinished == 0)
	{
		if (m_sVideoFrameOri.width != pFrame->width ||
			m_sVideoFrameOri.height != pFrame->height) {
			if (m_sVideoFrameOri.data)
			{
				delete[] m_sVideoFrameOri.data;
				m_sVideoFrameOri.data = NULL;
			}
		}

		m_sVideoFrameOri.width = pFrame->width;
		m_sVideoFrameOri.height = pFrame->height;
		m_sVideoFrameOri.pts = pFrame->pts;
		m_sVideoFrameOri.isKey = pFrame->key_frame;
		int ySize = pFrame->linesize[0] * pFrame->height;
		int uSize = pFrame->linesize[1] * pFrame->height >> 1;
		int vSize = pFrame->linesize[2] * pFrame->height >> 1;
		m_sVideoFrameOri.dataTotalLen = ySize + uSize + vSize;
		m_sVideoFrameOri.dataLen[0] = ySize;
		m_sVideoFrameOri.dataLen[1] = uSize;
		m_sVideoFrameOri.dataLen[2] = vSize;
		if (!m_sVideoFrameOri.data)
		{
			m_sVideoFrameOri.data = new uint8_t[m_sVideoFrameOri.dataTotalLen];
		}
		memcpy(m_sVideoFrameOri.data, pFrame->data[0], ySize);
		memcpy(m_sVideoFrameOri.data + ySize, pFrame->data[1], uSize);
		memcpy(m_sVideoFrameOri.data + ySize + uSize, pFrame->data[2], vSize);
		m_sVideoFrameOri.pitch[0] = pFrame->linesize[0];
		m_sVideoFrameOri.pitch[1] = pFrame->linesize[1];
		m_sVideoFrameOri.pitch[2] = pFrame->linesize[2];

		if (m_pCallback != NULL)
		{
			if (m_fScaleRatio < 0.9999f || m_fScaleRatio > 1.0001f) {
				scaleH264Data(&m_sVideoFrameOri);
				m_pCallback->outputVideo(&m_sVideoFrameScale, remoteName, remoteDeviceId);
			}
			else {
				m_pCallback->outputVideo(&m_sVideoFrameOri, remoteName, remoteDeviceId);
			}
		}
	}
	av_frame_free(&pFrame);

	return 0;
}

int FgAirplayServer::scaleH264Data(SFgVideoFrame* pSrcFrame)
{
	int nScreenWidth = pSrcFrame->width * m_fScaleRatio;
	int nScreenHeight = pSrcFrame->height * m_fScaleRatio;
	if (m_pSwsCtx && m_sVideoFrameScale.width != nScreenWidth || m_sVideoFrameScale.height != nScreenHeight)
	{
		sws_freeContext(m_pSwsCtx);
		m_pSwsCtx = NULL;
	}
	if (!m_pSwsCtx)
	{
		m_pSwsCtx = sws_getContext(pSrcFrame->width, pSrcFrame->height, AV_PIX_FMT_YUV420P,
			nScreenWidth, nScreenHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC /*SWS_POINT*/,
			NULL, NULL, NULL);
	}
	if (m_sVideoFrameScale.width != nScreenWidth || m_sVideoFrameScale.height != nScreenHeight)
	{
		delete[] m_sVideoFrameScale.data;
		m_sVideoFrameScale.data = NULL;
	}

	m_sVideoFrameScale.width = nScreenWidth;
	m_sVideoFrameScale.height = nScreenHeight;
	m_sVideoFrameScale.pts = pSrcFrame->pts;
	m_sVideoFrameScale.isKey = pSrcFrame->isKey;
	m_sVideoFrameScale.pitch[0] = ((nScreenWidth + 15) >> 4) << 4;
	m_sVideoFrameScale.pitch[1] = (((nScreenWidth >> 1) + 15) >> 4) << 4;
	m_sVideoFrameScale.pitch[2] = m_sVideoFrameScale.pitch[1];

	int ySize = m_sVideoFrameScale.pitch[0] * nScreenHeight;
	int uSize = m_sVideoFrameScale.pitch[1] * nScreenHeight >> 1;
	int vSize = m_sVideoFrameScale.pitch[2] * nScreenHeight >> 1;
	m_sVideoFrameScale.dataTotalLen = ySize + uSize + vSize;
	m_sVideoFrameScale.dataLen[0] = ySize;
	m_sVideoFrameScale.dataLen[1] = uSize;
	m_sVideoFrameScale.dataLen[2] = vSize;
	if (!m_sVideoFrameScale.data) {
		m_sVideoFrameScale.data = new uint8_t[m_sVideoFrameScale.dataTotalLen];
	}

	const uint8_t* srcSlice[3] = { pSrcFrame->data, pSrcFrame->data+ pSrcFrame->dataLen[0], pSrcFrame->data + pSrcFrame->dataLen[0] + pSrcFrame->dataLen[1] };
	const uint8_t* dst[3] = { m_sVideoFrameScale.data, 
		m_sVideoFrameScale.data + m_sVideoFrameScale.dataLen[0], 
		m_sVideoFrameScale.data + m_sVideoFrameScale.dataLen[0] + m_sVideoFrameScale.dataLen[1] };
	sws_scale(m_pSwsCtx, (const uint8_t* const*)srcSlice, (const int*)pSrcFrame->pitch, 0,
		pSrcFrame->height, (uint8_t* const*)dst, (const int*)m_sVideoFrameScale.pitch);

	return 0;
}

BOOL GetMacAddress(char strMac[6])
{
	PIP_ADAPTER_INFO pAdapterInfo;
	DWORD AdapterInfoSize;
	DWORD Err;
	AdapterInfoSize = 0;
	Err = GetAdaptersInfo(NULL, &AdapterInfoSize);
	if ((Err != 0) && (Err != ERROR_BUFFER_OVERFLOW)) {
		printf("获得网卡信息失败!");
		return   FALSE;
	}
	//   分配网卡信息内存  
	pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, AdapterInfoSize);
	if (pAdapterInfo == NULL) {
		printf("分配网卡信息内存失败");
		return   FALSE;
	}
	if (GetAdaptersInfo(pAdapterInfo, &AdapterInfoSize) != 0) {
		printf("获得网卡信息失败!\n");
		GlobalFree(pAdapterInfo);
		return   FALSE;
	}
	for (int i = 0; i < 6; i++)
	{
		strMac[i] = pAdapterInfo->Address[i];
	}
// 	strMac[0] = pAdapterInfo->Address[0];
// 	sprintf_s(strMac, 7, "%02X%02X%02X%02X%02X%02X",
// 		pAdapterInfo->Address[0],
// 		pAdapterInfo->Address[1],
// 		pAdapterInfo->Address[2],
// 		pAdapterInfo->Address[3],
// 		pAdapterInfo->Address[4],
// 		pAdapterInfo->Address[5]);

	GlobalFree(pAdapterInfo);
	return   TRUE;
}