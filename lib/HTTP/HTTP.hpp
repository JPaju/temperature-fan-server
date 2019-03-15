#ifndef HTTP_h
#define HTTP_h

#include <EthernetClient.h>

enum class HTTPMethod
{
	GET,
	POST,
	PUT,
	DELETE,
	UNKNOWN
};

enum class HTTPResponseType
{
	HTTP_200_OK,
	HTTP_201_CREATED,
	HTTP_204_NO_CONTENT,
	HTTP_400_BAD_REQUEST,
	HTTP_404_NOT_FOUND
};

class HTTP
{
public:

	static void sendHttpResponse(EthernetClient& client, HTTPResponseType type);
	static int parseRequestParameterIntValue(const String& request, const String& parameter);
	static String parseRequestPath(const String& request, int pathIndex);
	static HTTPMethod getRequestMethod(const String& request);

private:
	HTTP() {}
};


#endif
