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

	m_mutexMap = CreateMutex(NULL, FALSE, NULL);
}

FgAirplayServer::~FgAirplayServer()
{
	m_pCallback = NULL;

	clearChannels();

	CloseHandle(m_mutexMap);
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

	clearChannels();
	m_pCallback = NULL;
}

float FgAirplayServer::setScale(float fRatio)
{
	m_fScaleRatio = min(10, max(0.1, fRatio));

	FgAirplayChannelMap::iterator it;
	for (it = m_mapChannel.begin(); it != m_mapChannel.end(); ++it)
	{
		it->second->setScale(m_fScaleRatio);
	}
	return m_fScaleRatio;
}

void FgAirplayServer::clearChannels()
{
	CAutoLock oLock(m_mutexMap, "clearChannels");
	while (m_mapChannel.size() > 0)
	{
		FgAirplayChannelMap::iterator it = m_mapChannel.begin();
		it->second->release();
		m_mapChannel.erase(it);
	}
}

FgAirplayChannel* FgAirplayServer::getChannel(const char* remoteDeviceId)
{
	std::string deviceId(remoteDeviceId);
	FgAirplayChannel* pChannel = m_mapChannel[deviceId];
	if (NULL == pChannel)
	{
		pChannel = new FgAirplayChannel(m_pCallback);
		m_mapChannel[deviceId] = pChannel;
	}

	return pChannel;
}


void FgAirplayServer::connected(void* cls, const char* remoteName, const char* remoteDeviceId)
{
	FgAirplayServer* pServer = (FgAirplayServer*)cls;
	if (!pServer)
	{
		return;
	}
	CAutoLock oLock(pServer->m_mutexMap, "connected");
	pServer->getChannel(remoteDeviceId);

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

	CAutoLock oLock(pServer->m_mutexMap, "disconnected");
	std::string deviceId(remoteDeviceId);
	FgAirplayChannel* pChannel = pServer->m_mapChannel[std::string(remoteDeviceId)];
	if (pChannel) {
		pServer->m_mapChannel.erase(deviceId);
		pChannel->release();
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

	FgAirplayChannel* pChannel = NULL;
	{
		CAutoLock oLock(pServer->m_mutexMap, "video_process");
		pChannel = pServer->getChannel(remoteDeviceId);
		if (pChannel) {
			pChannel->addRef();
		}
	}
	if (pChannel)
	{
		pChannel->decodeH264Data(pData, remoteName, remoteDeviceId);
		pChannel->release();
	}
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