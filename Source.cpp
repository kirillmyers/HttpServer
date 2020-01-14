#define _LOG_

#include "HttpServer.h"
#include <Windows.h>
#include "Header.h"

LPCSTR patterns[1]
{
	"LoginWithSteam",
};

char* ClientConnectResponse(char*);

void* responses[1]
{
	ClientConnectResponse,
};

int main()
{
	TCPServer* test_server = new TCPServer;
	test_server->start_server("127.0.0.1", "80", patterns, responses, 1);

	return 0;
}

char* ClientConnectResponse(char* Request)
{
	return LoginWithSteamData;
}

char* AuthentificateSteamResponse(char* Request)
{
	if (strstr(Request, "SuckADick"))
	{
		return (char*)"no";
	}
	return (char*)"yes";
}