#include "HttpRequestHandler.hpp"


HttpRequestHandler::HttpRequestHandler()
: _serverPaths{}, _serverPathCount(0)
{
}

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

bool HttpRequestHandler::addRoute(const String& path, ArduinoServerInterface &obj)
{
	if (_serverPathCount < MAX_SERVERS && pathNotInUse(path)) {
		_serverPaths[_serverPathCount] = {path, &obj};
		_serverPathCount++;
		return true;
	}
	return false;
}

bool HttpRequestHandler::removeRoute(const String &path)
{
	for (int i=0; i<_serverPathCount; i++) {
		if (_serverPaths[i].path.equals(path)) {
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

void HttpRequestHandler::run()
{
	EthernetClient client = _ethServer.available();
	if (client) {
		handleRequest(client);
	}
	Ethernet.maintain();
}

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

void HttpRequestHandler::getFirstRequestLine(EthernetClient &client, String& outRequest) {
	while (!outRequest.endsWith("\n") && client.available()) {
		char c = client.read();
		outRequest += c;
	}
	//Read rest of the request from network card's buffer
	while (client.available()) client.read();
}

bool HttpRequestHandler::pathNotInUse(const String& path)
{
	for (int i=0; i<_serverPathCount; i++) {
		if (_serverPaths[i].path.equals(path)) {
			return false;
		}
	}
	return true;
}
