#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "airplay.h"
#include "raop_rtp.h"
//#include "rsakey.h"
#include "digest.h"
#include "httpd.h"	
#include "sdp.h"
#include "global.h"
#include "utils.h"
#include "netutils.h"
#include "logger.h"
#include "compat.h"
//to do fairplay
//#include "li"
#include "pairing.h"
#include "fairplay.h"

#define MAX_SIGNATURE_LEN 512

#define MAX_PASSWORD_LEN 64

/* MD5 as hex fits here */
#define MAX_NONCE_LEN 32

#define MAX_PACKET_LEN 4096

const __int64 DELTA_EPOCH_IN_MICROSECS = 11644473600000000;


struct airplay_s
{
	airplay_callbacks_t callbacks;

	logger_t *logger;
	pairing_t* pairing;

	httpd_t *httpd;
	//rsakey_t *rsakey;

	httpd_t *mirror_server;

	/* Hardware address information */
	unsigned char hwaddr[MAX_HWADDR_LEN];
	int hwaddrlen;

	/* Password information */
	char password[MAX_PASSWORD_LEN + 1];
};

struct airplay_conn_s
{
	airplay_t *airplay;
	raop_rtp_t *airplay_rtp;
	fairplay_t* fairplay;
	pairing_session_t* pairing;

	unsigned char *local;
	int locallen;

	unsigned char *remote;
	int remotelen;

	char nonce[MAX_NONCE_LEN + 1];

	unsigned char aeskey[16];
	unsigned char iv[16];
	unsigned char buffer[MAX_PACKET_LEN];
	int pos;

};

typedef struct airplay_conn_s airplay_conn_t;

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK                  200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NEED_AUTH           401
#define AIRPLAY_STATUS_NOT_FOUND           404
#define AIRPLAY_STATUS_METHOD_NOT_ALLOWED  405
#define AIRPLAY_STATUS_PRECONDITION_FAILED 412
#define AIRPLAY_STATUS_NOT_IMPLEMENTED     501
#define AIRPLAY_STATUS_NO_RESPONSE_NEEDED  1000

#define EVENT_NONE     -1
#define EVENT_PLAYING   0
#define EVENT_PAUSED    1
#define EVENT_LOADING   2
#define EVENT_STOPPED   3
const char *eventStrings[] = { "playing", "paused", "loading", "stopped" };

#define STREAM_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>width</key>\r\n"\
"<integer>1280</integer>\r\n"\
"<key>height</key>\r\n"\
"<integer>720</integer>\r\n"\
"<key>version</key>\r\n"\
"<string>110.92</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define PLAYBACK_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>duration</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>loadedTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%f</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"<key>playbackBufferEmpty</key>\r\n"\
"<true/>\r\n"\
"<key>playbackBufferFull</key>\r\n"\
"<false/>\r\n"\
"<key>playbackLikelyToKeepUp</key>\r\n"\
"<true/>\r\n"\
"<key>position</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>rate</key>\r\n"\
"<real>%f</real>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<true/>\r\n"\
"<key>seekableTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%f</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define PLAYBACK_INFO_NOT_READY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<false/>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define SERVER_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>deviceid</key>\r\n"\
"<string>%s</string>\r\n"\
"<key>features</key>\r\n"\
"<integer>119</integer>\r\n"\
"<key>model</key>\r\n"\
"<string>Kodi,1</string>\r\n"\
"<key>protovers</key>\r\n"\
"<string>1.0</string>\r\n"\
"<key>srcvers</key>\r\n"\
"<string>"GLOBAL_VERSION"</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define EVENT_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>category</key>\r\n"\
"<string>video</string>\r\n"\
"<key>sessionID</key>\r\n"\
"<integer>%d</integer>\r\n"\
"<key>state</key>\r\n"\
"<string>%s</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"\

#define AUTH_REALM "AirPlay"
#define AUTH_REQUIRED "WWW-Authenticate: Digest realm=\""  AUTH_REALM  "\", nonce=\"%s\"\r\n"

#include "airplay_handlers.h"

static void *
conn_init(void *opaque, unsigned char *local, int locallen, unsigned char *remote, int remotelen)
{
	airplay_t* airplay = opaque;
	airplay_conn_t *conn;

	conn = calloc(1, sizeof(airplay_conn_t));
	if (!conn)
	{
		return NULL;
	}
	conn->airplay = airplay;
	conn->airplay_rtp = NULL;

	conn->fairplay = fairplay_init(airplay->logger);
	//fairplay_init2();
	if (!conn->fairplay) {
		free(conn);
		return NULL;
	}

	conn->pairing = pairing_session_init(airplay->pairing);
	if (!conn->pairing) {
		fairplay_destroy(conn->fairplay);
		free(conn);
		return NULL;
	}

	if (locallen==4)
	{
		logger_log(conn->airplay->logger, LOGGER_INFO,
			"Local: %d.%d.%d.%d",
			local[0], local[1], local[2], local[3]);
	}
	else if (locallen == 16) {
		logger_log(conn->airplay->logger, LOGGER_INFO,
			"Local: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
			local[0], local[1], local[2], local[3], local[4], local[5], local[6], local[7],
			local[8], local[9], local[10], local[11], local[12], local[13], local[14], local[15]);
	}
	if (remotelen == 4) {
		logger_log(conn->airplay->logger, LOGGER_INFO,
			"Remote: %d.%d.%d.%d",
			remote[0], remote[1], remote[2], remote[3]);
	}
	else if (remotelen == 16) {
		logger_log(conn->airplay->logger, LOGGER_INFO,
			"Remote: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
			remote[0], remote[1], remote[2], remote[3], remote[4], remote[5], remote[6], remote[7],
			remote[8], remote[9], remote[10], remote[11], remote[12], remote[13], remote[14], remote[15]);
	}

	conn->local = malloc(locallen);
	assert(conn->local);
	memcpy(conn->local, local, locallen);

	conn->remote = malloc(remotelen);
	assert(conn->remote);
	memcpy(conn->remote, remote, remotelen);

	conn->locallen = locallen;
	conn->remotelen = remotelen;

	digest_generate_nonce(conn->nonce, sizeof(conn->nonce));
	return conn;

}

static void 
conn_request(void *ptr, http_request_t *request, http_response_t **response)
{
	const char realm[] = "airplay";
	airplay_conn_t *conn = ptr;
	airplay_t *airplay = conn->airplay;

	const char *method;
	const char *cseq;
	//const char *challenge;
	int require_auth = 0;
	char* response_data = NULL;
	int response_datalen = 0;
	int responseLength = 0;

	const char * url = http_request_get_url(request);
	method = http_request_get_method(request);
	cseq = http_request_get_header(request, "CSeq");

	if (!method)
	{
		return;
	}

	*response = http_response_init("RTSP/1.0", 200, "OK");
	if (cseq != NULL) {
		http_response_add_header(*response, "CSeq", cseq);
	}
	//http_response_add_header(*response, "Apple-Jack-Status", "connected; type=analog");
	http_response_add_header(*response, "Server", "AirTunes/220.68");

	const char * contentType = http_request_get_header(request, "content-type");
	const char * m_sessionId = http_request_get_header(request, "x-apple-session-id");
	const char * authorization = http_request_get_header(request, "authorization");
	const char * photoAction = http_request_get_header(request, "x-apple-assetaction");
	const char * photoCacheId = http_request_get_header(request, "x-apple-assetkey");

	int status = AIRPLAY_STATUS_OK;
	int needAuth = 0;

	logger_log(conn->airplay->logger, LOGGER_DEBUG, "[AirPlay] Handling request %s with URL %s", method, url);

	{
		const char *data;
		int len;
		data = http_request_get_data(request, &len);
		logger_log(conn->airplay->logger, LOGGER_DEBUG, "data len %d:%s\n", len, data);
	}

	airplay_handler_t handler = NULL;
	if (!strcmp(method, "POST") && !strcmp(url, "/pair-setup")) {
		handler = &airplay_handler_pairsetup;
	}
	else if (!strcmp(method, "POST") && !strcmp(url, "/pair-verify")) {
		handler = &airplay_handler_pairverify;
	}
	else if (!strcmp(method, "GET") && !strcmp(url, "/server-info")) {
		handler = &airplay_handler_serverinfo;
	}
	else if (!strcmp(method, "POST") && !strcmp(url, "/play")) {
		handler = &airplay_handler_play;
	}
	else if (!strcmp(method, "PUT") && !strncmp(url, "/setProperty", strlen("/setProperty"))) {
//		handler = &raop_handler_pairsetup;
	}
	// POST /rate?value=1.000000 HTTP/1.1
	else if (!strcmp(method, "POST") && !strncmp(url, "/rate", strlen("/rate"))) {
//		handler = &raop_handler_pairsetup;
	}
	else if (!strcmp(method, "GET") && !strcmp(url, "/playback-info")) {
		handler = &airplay_handler_playbackinfo;
	}
	else if (!strcmp(method, "POST") && !strcmp(url, "/reverse")) {
		http_response_destroy(*response);
		*response = http_response_init("HTTP/1.1", 101, "Switching Protocols");
		http_response_add_header(*response, "Upgrade", "PTTH/1.0");
		http_response_add_header(*response, "Connection", "Upgrade");
	}

	if (handler != NULL) {
		handler(conn, request, *response, &response_data, &response_datalen);
	}

	http_response_finish(*response, response_data, response_datalen);
	if (response_data) {
		free(response_data);
		response_data = NULL;
		response_datalen = 0;
	}
}

static void 
conn_destroy(void *ptr)
{
	airplay_conn_t *conn = ptr;
	if (conn->airplay_rtp)
	{
		raop_rtp_destroy(conn->airplay_rtp);
	}
	free(conn->local);
	free(conn->remote);
	pairing_session_destroy(conn->pairing);
	fairplay_destroy(conn->fairplay);
	free(conn);
}

static void 
conn_datafeed(void *ptr, unsigned char *data, int len)
{
	int size;
	unsigned short type;
	unsigned short type1;

	airplay_conn_t *conn = ptr;
	size = *(int*)data;
	type = *(unsigned short*)(data + 4);
	type1 = *(unsigned short*)(data + 6);

	logger_log(conn->airplay->logger, LOGGER_DEBUG, "Add data size=%d type %2x %2x", size, type, type1);
}
 
airplay_t *
airplay_init(int max_clients, airplay_callbacks_t *callbacks, const char *pemkey, int *error)
{
	airplay_t *airplay;
	pairing_t* pairing;
	httpd_t *httpd;
	httpd_t *mirror_server;
	//rsakey_t *rsakey;
	httpd_callbacks_t httpd_cbs;

	assert(callbacks);
	assert(max_clients > 0);
	assert(max_clients < 100);
	//assert(pemkey);

	if (netutils_init()<0)
	{
		return NULL;
	}

// 	if (!callbacks->audio_init||
// 		!callbacks->audio_process||
// 		!callbacks->audio_destroy)
// 	{
// 		return NULL;
// 	}

	airplay = calloc(1, sizeof(airplay_t));
	if (!airplay)
	{
		return NULL;
	}

	airplay->logger = logger_init();
	pairing = pairing_init_generate();
	if (!pairing) {
		free(airplay);
		return NULL;
	}

	memset(&httpd_cbs, 0, sizeof(httpd_cbs));
	httpd_cbs.opaque = airplay;
	httpd_cbs.conn_init = &conn_init;
	httpd_cbs.conn_request = &conn_request;
	httpd_cbs.conn_destroy = &conn_destroy;
	// httpd_cbs.conn_datafeed = &conn_datafeed;

	httpd = httpd_init(airplay->logger, &httpd_cbs, max_clients);
	if (!httpd)
	{
		pairing_destroy(pairing);
		free(airplay);
		return NULL;
	}

	mirror_server = httpd_init(airplay->logger, &httpd_cbs, max_clients);
	if (!mirror_server)
	{
		free(httpd);
		free(airplay);
		return NULL;
	}

	memcpy(&airplay->callbacks, callbacks, sizeof(airplay_callbacks_t));

	/* Initialize RSA key handler */
	//rsakey = rsakey_init_pem(pemkey);
	//if (!rsakey) {
	//	free(httpd);
	//	free(mirror_server);
	//	free(airplay);
	//	return NULL;
	//}

	airplay->pairing = pairing;
	airplay->httpd = httpd;
	//airplay->rsakey = rsakey;

	airplay->mirror_server = mirror_server;

	return airplay;
}


airplay_t *
airplay_init_from_keyfile(int max_clients, airplay_callbacks_t *callbacks, const char *keyfile, int *error)
{
	airplay_t *airplay;
	char *pemstr;

	if (utils_read_file(&pemstr,keyfile)<0)
	{
		return NULL;
	}
	airplay = airplay_init(max_clients, callbacks, pemstr, error);
	free(pemstr);
	return airplay;
}

int
airplay_is_running(airplay_t *airplay)
{
	assert(airplay);

	return httpd_is_running(airplay->httpd);
}

void
airplay_set_log_level(airplay_t *airplay, int level)
{
	assert(airplay);

	logger_set_level(airplay->logger, level);
}

void
airplay_set_log_callback(airplay_t *airplay, airplay_log_callback_t callback, void *cls)
{
	assert(airplay);

	logger_set_callback(airplay->logger, callback, cls);
}

int airplay_start(airplay_t *airplay, unsigned short *port, const char *hwaddr,
	int hwaddrlen, const char *password)
{
	int ret;
	unsigned short mirror_port;

	assert(airplay);
	assert(port);
	assert(hwaddr);

	if (hwaddrlen>MAX_HWADDR_LEN)
	{
		return -1;
	}

	memset(airplay->password, 0, sizeof(airplay->password));
	if (password)
	{
		if (strlen(password) > MAX_PASSWORD_LEN)
		{
			return -1;
		}

		strncpy(airplay->password, password, MAX_PASSWORD_LEN);
	}

	memcpy(airplay->hwaddr, hwaddr, hwaddrlen);
	airplay->hwaddrlen = hwaddrlen;

	ret = httpd_start(airplay->httpd, port);
	if (ret != 1) return ret;

	mirror_port = 7100;
	ret = httpd_start(airplay->mirror_server, &mirror_port);
	return ret;
}

void airplay_stop(airplay_t *airplay)
{
	assert(airplay);

	httpd_stop(airplay->httpd);
	httpd_stop(airplay->mirror_server);
}

void airplay_destroy(airplay_t* airplay) {
	if (airplay) {
		airplay_stop(airplay);
		//rsakey_destroy(airplay->rsakey);
		httpd_destroy(airplay->httpd);
		httpd_destroy(airplay->mirror_server);
		pairing_destroy(airplay->pairing);
		logger_destroy(airplay->logger);
		free(airplay);

		/* Cleanup the network */
		netutils_cleanup();
	}
}
