#include "HttpRequestHandler.hpp"


HttpRequestHandler::HttpRequestHandler()
: _serverPaths{}, _serverPathCount(0)
{
}

/**
	Initalizes server and requests a DHCP-lease.

	@param portNumber: Number of the port where the server runs
	@param macAddress: MAC-address used for DHCP-request, byte[6]
*/
void HttpRequestHandler::init(int portNumber, byte* macAddress)
{
	Serial.println();
	Serial.println(F("Trying to obtain DHCP-lease"));
	Ethernet.begin(macAddress);
	Serial.print(F("Received DHCP-lease, IP-address: "));
	Serial.println(Ethernet.localIP());

	_ethServer = EthernetServer(portNumber);
	_requestBuffer = String(REQUEST_BUFFER_SIZE);
}

/**
	Adds new HttpRequestHandler::ServerPath to _serverPaths.

	@param path: Non-empty String
	@param server: Server that handles the requests to path
	@return true if addition of the route was successful, otherwise returns false
*/
bool HttpRequestHandler::addRoute(const String& path, ArduinoServerInterface &server)
{
	if (_serverPathCount < MAX_SERVERS && pathNotInUse(path)) {
		_serverPaths[_serverPathCount] = {path, &server};
		_serverPathCount++;
		return true;
	}
	return false;
}

/**
	Removes server from _serverPaths and calls it's destructor.

	@param path: Server route to be removed, Non-empty String
	@return true if removing the server was successful, otherwise false
*/
bool HttpRequestHandler::removeRoute(const String &path)
{
	for (int i=0; i<_serverPathCount; i++) {
		if (_serverPaths[i].path.equals(path)) {
			_serverPaths[i].server->~ArduinoServerInterface();

			if (!(i == _serverPathCount -1)) {
				//Move every serverPath backwards one index
				for (int j=i; j<_serverPathCount; j++) {
					_serverPaths[j] = _serverPaths[j + 1];
				}
			}
			_serverPathCount--;
			return true;
		}
	}
	return false;
}

/**
	Checks for new clients and handles request when present. Also maintains DHCP-lease.
	Intended to call run-method from the main-loop.
*/
void HttpRequestHandler::run()
{
	EthernetClient client = _ethServer.available();
	if (client) {
		handleRequest(client);
	}
	Ethernet.maintain();
}

/**
	Parses the request-path and directs the request to the correct server.
	If the path is not recognized, sends 404 Not found to client.

	@param client: EthernetClient where the request originated
*/
void HttpRequestHandler::handleRequest(EthernetClient& client)
{
	getFirstRequestLine(client, _requestBuffer);

	Serial.println(F("First line of request:"));
	Serial.println(_requestBuffer);
	Serial.println();

	//Test that assumed request isn't actually response
	if (_requestBuffer.indexOf(F("HTTP")) > 3) {
		String path = HTTP::parseRequestPath(_requestBuffer, 1);
		if (!(passRequestToServer(path, _requestBuffer, client))) {
			HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_404_NOT_FOUND);
		}
		client.stop();
		_requestBuffer.remove(0);
	}
}

/**
	Passes request and client to server. Goes through _serverPaths and tries to find matching path.

	@param path: Path to server, Non-empty String
	@param request:	First line of the HTTP-request
	@param client: Client where the request originated from
	@return True if server with correct path was found, false if not
*/
bool HttpRequestHandler::passRequestToServer(const String &path, const String &request, EthernetClient &client)
{
	for (int i=0; i<_serverPathCount; i++) {
		if (_serverPaths[i].path.equals(path)) {
			_serverPaths[i].server->handleRequest(request, client);
			return true;
		}
	}
	return false;
}

/**
	Parses the first line of a HTTP-request from a Ethernet-client.

	@param client: EthernetClient where the request originates
	@param outRequest: String where the first line of the request is stored, "Output variable"
*/
void HttpRequestHandler::getFirstRequestLine(EthernetClient &client, String& outRequest) {
	while (!outRequest.endsWith("\n") && client.available()) {
		char c = client.read();
		outRequest += c;
	}
	//Read rest of the request from network card's buffer
	while (client.available()) client.read();
}

/**
	Checks if a path is already in use.

	@param path: Path to be tested
	@return True if path is free to use, False if it's already in use
*/
bool HttpRequestHandler::pathNotInUse(const String& path)
{
	for (int i=0; i<_serverPathCount; i++) {
		if (_serverPaths[i].path.equals(path)) {
			return false;
		}
	}
	return true;
}
