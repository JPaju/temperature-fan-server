#ifndef HttpRequestHandler_h
#define HttpRequestHandler_h

#define MAX_SERVERS 3
#define REQUEST_BUFFER_SIZE 80

#include <Ethernet2.h>
#include "ArduinoServerInterface.hpp"
#include "HTTP.hpp"

struct ServerPath
{
	String path;
	ArduinoServerInterface* server;
};

class HttpRequestHandler
{

public:

	HttpRequestHandler();
	void init(int portNumber, byte* macAddress);
	bool addRoute(const String& path, ArduinoServerInterface& obj);
	bool removeRoute(const String& path);
	void run();

private:

	EthernetServer _ethServer = 0;
	ServerPath _serverPaths[MAX_SERVERS];
	int _serverPathCount;
	String _requestBuffer;

	void handleRequest(EthernetClient& client);
	bool passRequestToServer(const String& path, const String &request, EthernetClient &client);
	void getFirstRequestLine(EthernetClient& client, String& outRequest);
	bool pathNotInUse(const String& path);
};

#endif
