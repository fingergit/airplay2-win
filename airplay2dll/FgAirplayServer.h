#pragma once
#include "Airplay2Head.h"
#include "stream.h"
#include <map>
#include <string>

#include "dnssd.h"
#include "airplay.h"
#include "raop.h"
#include "FgAirplayChannel.h"

typedef std::map<std::string, FgAirplayChannel*> FgAirplayChannelMap;

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
	void clearChannels();
	FgAirplayChannel* getChannel(const char* remoteDeviceId);

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
	IAirServerCallback*		m_pCallback;

	dnssd_t*				m_pDnsSd;
	airplay_t*				m_pAirplay;
	raop_t*					m_pRaop;

	airplay_callbacks_t		m_stAirplayCB;
	raop_callbacks_t		m_stRaopCB;

// 	volatile bool			m_bVideoQuit;

	void*					m_mutexMap;

	float					m_fScaleRatio;
	FgAirplayChannelMap		m_mapChannel;
};

