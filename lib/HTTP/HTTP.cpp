#include "HTTP.hpp"

#define HTTP_GET F("GET")
#define HTTP_POST F("POST")
#define HTTP_PUT F("PUT")
#define HTTP_DELETE F("DELETE")

#define HTTP_OK F("HTTP/1.1 200 OK \r\nContent-Type: application/json \r\nConnection: Closed\r\n")
#define HTTP_CREATED F("HTTP/1.1 201 Created \r\nContent-Type: application/json \r\nConnection: Closed\r\n")
#define HTTP_NO_CONTENT F("HTTP/1.1 204 No Content \r\nConnection: Closed\r\n")
#define HTTP_BAD_REQUEST F("HTTP/1.1 400 Bad Request \r\nContent-Type: application/json \r\nConnection: Closed \r\n\r\n{ \"error\" : \"Missing or invalid Parameters\" }\r\n")
#define HTTP_NOT_FOUND F("HTTP/1.1 404 Not Found\r\n Content-Type: text/html \r\nConnection: Closed \r\n\r\n{ \"error\" : \"Path not found\" }\r\n")


/**
	Sends HTTP 1.1-response to client.

	@param client: Client whom the response is sent to
	@param type: HTTP-response type
*/
void HTTP::sendHttpResponse(EthernetClient& client, HTTPResponseType type) {
	switch (type) {
		case HTTPResponseType::HTTP_200_OK:
			client.println(HTTP_OK);
			break;
		case HTTPResponseType::HTTP_201_CREATED:
			client.println(HTTP_CREATED);
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

/**
	Finds a parameter int value from a first line of a HTTP-request.

	@param request: String from which the parameter is parsed from
	@param parameter: String that specifies the parameter to search
	@return Value of the parameter. If no parameter found, returns -1.

	Example:
	request = "GET /example?foo=1&bar=2 HTTP/1.1"
	parameter = "bar"
	returns: 2
*/
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

		//Check that return value is actually zero and not
		//just invalid conversion from String.toInt() (returns 0 if value invalid)
		if (returnValue == 0 && ((valueEndIndex - valueStartIndex != 1) || (request.charAt(valueStartIndex) != '0')))
			returnValue = -1;
		return returnValue;
	}
	return -1;
}

/**
	Parses a path from a first line of a HTTP-request.

	@param request: String from which the path is parsed from
	@param pathDepth:
	@return Path as a String. If no path is found at specified depth, empty String is returned.

	Example:
	request = "GET /example/foo/bar HTTP/1.1"
	pathDepth = 2
	returns "foo"
*/
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

/**
	Parses the HTTP-method from a HTTP-request.

	@param request: String containing first line of a HTTP-request
	@return HTTPMethod-enum. If no method is recognized, HTTPMethod::UNKNOWN is returned.
*/
HTTPMethod HTTP::getRequestMethod(const String &request) {
	String method = request.substring(0, request.indexOf(' '));

	if (method.indexOf(HTTP_GET) > -1) return HTTPMethod::GET;
	if (method.indexOf(HTTP_POST) > -1) return HTTPMethod::POST;
	if (method.indexOf(HTTP_PUT) > -1) return HTTPMethod::PUT;
	if (method.indexOf(HTTP_DELETE) > -1) return HTTPMethod::DELETE;

	return HTTPMethod::UNKNOWN;
}
