#ifndef ArduinoServerInterface_h
#define ArduinoServerInterface_h
#include <EthernetClient.h>

class ArduinoServerInterface {
public:
	virtual void handleRequest(const String& request, EthernetClient& client) = 0;
};

#endif
