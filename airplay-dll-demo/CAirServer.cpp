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

void CAirServer::start(CSDLPlayer* pPlayer)
{
    stop();
    m_pCallback->setPlayer(pPlayer);
    m_pServer = fgServerStart("FgAirplay", m_pCallback);
}

void CAirServer::stop()
{
    if (m_pServer != NULL) {
        fgServerStop(m_pServer);
        m_pServer = NULL;
    }
}
