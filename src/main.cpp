#include "HttpRequestHandler.hpp"
#include "TemperatureServer.hpp"
#include "FanServer.hpp"

#define TEMPSENSOR_PIN 4
#define HTTP_SERVER_PORT 80

static 	byte mac[6]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

HttpRequestHandler httpHandler;
TemperatureServer tempServer(TEMPSENSOR_PIN);
FanServer fanServer;


void setup() {
	Serial.begin(9600);
	while(!Serial);

	httpHandler.init(HTTP_SERVER_PORT,mac);
	httpHandler.addRoute(FANSERVER_PATH, fanServer);
	httpHandler.addRoute(TEMPERATURE_SERVER_PATH, tempServer);
}


void loop()
{
	httpHandler.run();
}
