#include "TemperatureServer.hpp"
#include "HTTP.hpp"

#define JSON_BUFFER_SIZE 300 //Enough for 4 temperaturesensors
#define UPDATE_TEMPS_PATH F("updatetemps")
#define UPDATE_SENSORS_PATH F("updatesensors")

#define ID_ATTRIBUTE F("id")
#define TEMPERATURE_ATTRIBUTE F("temperature")


TemperatureServer::TemperatureServer(int sensorPin)
:bus(sensorPin), sensors(&bus)
{
	sensors.begin();
	updateTemperatures();
}

void TemperatureServer::handleRequest(const String &request, EthernetClient &client) {
	String path = HTTP::parseRequestPath(request, 2);
	HTTPMethod method = HTTP::getRequestMethod(request);

	if (method == HTTPMethod::GET && path.length() < 1) return this->getTemperatures(client);
	else if (path.equals(UPDATE_TEMPS_PATH) && method == HTTPMethod::PUT) return this->updateTemperatures(client);
	else if (path.equals(UPDATE_SENSORS_PATH) && method == HTTPMethod::PUT) return this->updateSensors(client);

	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_404_NOT_FOUND);
}

void TemperatureServer::updateTemperatures(EthernetClient& client)
{
	updateTemperatures();
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
}

void TemperatureServer::updateSensors(EthernetClient& client)
{
	updateSensors();
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
}

void TemperatureServer::getTemperatures(EthernetClient& client)
{
	updateTemperatures();
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	JsonObject& data = root.createNestedObject(F("data"));
	JsonArray& tempSensors = data.createNestedArray(F("tempsensors"));

	for (int i=0; i<sensors.getDeviceCount(); i++) {
		JsonObject& tempSensor = tempSensors.createNestedObject();
		char id[64] = "";
		getID(i, id);
		tempSensor[ID_ATTRIBUTE] = id;
		tempSensor[TEMPERATURE_ATTRIBUTE] = getTemperature(i);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_200_OK);
	root.printTo(client);
}

void TemperatureServer::updateTemperatures()
{
	sensors.requestTemperatures();
}

void TemperatureServer::updateSensors()
{
	sensors.begin();
}

float TemperatureServer::roundTemp(float f)
{
	return floor(f * 5 + 0.5) / 5;
}

float TemperatureServer::getTemperature(int index)
{
	return roundTemp(sensors.getTempCByIndex(index));
}

void TemperatureServer::getID(int index, char* str)
{
	byte address[8];
	sensors.getAddress(address, index);
	array_to_string(address, sizeof(address), str);
}

void TemperatureServer::array_to_string(byte array[], unsigned int len, char buffer[])
{
	for (unsigned int i = 0; i < len; i++) {
		byte nib1 = (array[i] >> 4) & 0x0F;
		byte nib2 = (array[i] >> 0) & 0x0F;
		buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
		buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
	}
	buffer[len*2] = '\0';
}
