#include "Airplay2Head.h"
#include "FgAirplayServer.h"

void* fgServerStart(const char serverName[AIRPLAY_NAME_LEN], IAirServerCallback* callback) {
	FgAirplayServer* pServer = new FgAirplayServer();
	pServer->start(serverName, callback);
	return pServer;
}

void fgServerStop(void* handle) {
	if (handle != NULL) {
		FgAirplayServer* pServer = (FgAirplayServer*)handle;
		delete pServer;
		pServer = NULL;
	}
}

