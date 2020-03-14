#include "CAirServer.h"
#include "CSDLPlayer.h"
#include "CAirServerCallback.h"

CAirServer::CAirServer()
	: m_pCallback(NULL)
    , m_pServer(NULL)
{
    m_pCallback = new CAirServerCallback();
}

CAirServer::~CAirServer()
{
    delete m_pCallback;
    stop();
}

bool getHostName(char hostName[512]) 
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	gethostname(hostName, 511);
	if (strlen(hostName) > 0)
	{
		return true;
	}
	else
	{
		DWORD n = 511;

		if (::GetComputerNameA(hostName, &n))
		{
			if (n > 2)
			{
				hostName[n] = '\0';
			}
		}
	}
}

void CAirServer::start(CSDLPlayer* pPlayer)
{
    stop();
    m_pCallback->setPlayer(pPlayer);
	char hostName[512];
	memset(hostName, 0, sizeof(hostName));
	getHostName(hostName);
	char serverName[1024] = { 0 };
	sprintf_s(serverName, 1024, "FgAirplay[%s]", hostName);
    m_pServer = fgServerStart(serverName, 5001, 7001, m_pCallback);
}

void CAirServer::stop()
{
    if (m_pServer != NULL) {
        fgServerStop(m_pServer);
        m_pServer = NULL;
    }
}

float CAirServer::setVideoScale(float fRatio)
{
    return fgServerScale(m_pServer, fRatio);
}
