#pragma once

class CSDLPlayer;
class CAirServerCallback;

class CAirServer
{
public:
	CAirServer();
	~CAirServer();

public:
	void start(CSDLPlayer* pPlayer);
	void stop();

private:
	CAirServerCallback* m_pCallback;
	void* m_pServer;
};

