#include "HTTP.hpp"

#define HTTP_GET F("GET")
#define HTTP_POST F("POST")
#define HTTP_PUT F("PUT")
#define HTTP_DELETE F("DELETE")

#define HTTP_OK F("HTTP/1.1 200 OK \r\nContent-Type: application/json \r\nConnection: Closed\r\n")
#define HTTP_NO_CONTENT F("HTTP/1.1 204 No Content \r\nConnection: Closed\r\n")
#define HTTP_BAD_REQUEST F("HTTP/1.1 400 Bad Request \r\nContent-Type: application/json \r\nConnection: Closed \r\n\r\n{ \"error\" : \"Missing or invalid Parameters\" }\r\n")
#define HTTP_NOT_FOUND F("HTTP/1.1 404 Not Found\r\n Content-Type: text/html \r\nConnection: Closed \r\n\r\n{ \"error\" : \"Path not found\" }\r\n")

void HTTP::sendHttpResponse(EthernetClient& client, HTTPResponseType type) {
	switch (type) {
		case HTTPResponseType::HTTP_200_OK:
			client.println(HTTP_OK);
			break;
		case HTTPResponseType::HTTP_204_NO_CONTENT:
			client.println(HTTP_NO_CONTENT);
			break;
		case HTTPResponseType::HTTP_400_BAD_REQUEST:
			client.println(HTTP_BAD_REQUEST);
			break;
		case HTTPResponseType::HTTP_404_NOT_FOUND:
			client.println(HTTP_NOT_FOUND);
			break;
	}
}


int HTTP::parseRequestParameterIntValue(const String &request, const String &parameter) {
	int parameterStartIndex = request.indexOf(parameter, request.indexOf("?"));
	int valueStartIndex = parameterStartIndex + parameter.length();

	if (parameterStartIndex > 0 && request.charAt(valueStartIndex) == '=') {
		int valueEndIndex;
		int limitIndex = request.indexOf(F("HTTP"), parameterStartIndex);

		//Go through every charachter after '=' until new parameter or end of request
		for (int i=++valueStartIndex; i<limitIndex; i++) {
			char c = request.charAt(i);
			 if (c == '&' || c == ' ') {
				valueEndIndex = i;
				break;
			}
		}
		int returnValue = request.substring(valueStartIndex, valueEndIndex).toInt();

		//Check that returnValue is actually zero and not just invalid conversion from String.toInt() (returns 0 if value invalid)
		if (returnValue == 0 && ((valueEndIndex - valueStartIndex != 1) || (request.charAt(valueStartIndex) != '0')))
			returnValue = -1;
		return returnValue;
	}
	return -1;
}

String HTTP::parseRequestPath(const String &request, int pathDepth) {
	int pathStartIndex = 0, pathEndIndex = 0;
	for (int i=0; i<pathDepth; i++) {
		pathStartIndex = request.indexOf('/', pathStartIndex)+1;
		if (pathStartIndex == 0) break;
	}
	if (pathStartIndex > 0) {
		int limitIndex = request.indexOf(F("HTTP"));
		for (int i=pathStartIndex; i<limitIndex; i++) {
			char c = request.charAt(i);
			if (c == '/' || c == '?' || c == ' ' || c == '&' || c == '=') {
				pathEndIndex = i;
				break;
			}
		}
		if (pathEndIndex < pathStartIndex) pathEndIndex = pathStartIndex = 0;
	}
	return request.substring(pathStartIndex, pathEndIndex);
}

HTTPMethod HTTP::getRequestMethod(const String &request) {
	String method = request.substring(0, request.indexOf(' '));

	if (method.indexOf(HTTP_GET) > -1) return HTTPMethod::GET;
	if (method.indexOf(HTTP_POST) > -1) return HTTPMethod::POST;
	if (method.indexOf(HTTP_PUT) > -1) return HTTPMethod::PUT;
	if (method.indexOf(HTTP_DELETE) > -1) return HTTPMethod::DELETE;

	return HTTPMethod::UNKNOWN;
}
