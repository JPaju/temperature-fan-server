#ifndef TemperatureServer_h
#define TemperatureServer_h

#define TEMPERATURE_SERVER_PATH F("temperatures")

#include <OneWire.h>
#include <DallasTemperature.h>
#include <EthernetClient.h>
#include <ArduinoJson.h>
#include "ArduinoServerInterface.hpp"

class TemperatureServer : public ArduinoServerInterface
{
public:

	TemperatureServer(int sensorPin);

	void handleRequest(const String& request, EthernetClient& client);
	void updateTemperatures(EthernetClient& client);
	void updateSensors(EthernetClient& client);
	void getTemperatures(EthernetClient& client);

private:

	OneWire bus;
	DallasTemperature sensors;

	void updateTemperatures();
	void updateSensors();
	float roundTemp(float f);
	float getTemperature(int index);
	void getID(int index, char* str);
	void array_to_string(byte array[], unsigned int len, char buffer[]);

};

#endif
