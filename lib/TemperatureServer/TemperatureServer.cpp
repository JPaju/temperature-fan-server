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

/**
	Handles HTTP-request. If does not recognize request path and/or method, sends 404 Not found.

	@param request: First line of a HTTP-request
	@param client: Client to which the response is sent
*/
void TemperatureServer::handleRequest(const String &request, EthernetClient &client)
{
	String path = HTTP::parseRequestPath(request, 2);
	HTTPMethod method = HTTP::getRequestMethod(request);

	if (method == HTTPMethod::GET && path.length() < 1) return getTemperatures(client);
	else if (path.equals(UPDATE_TEMPS_PATH) && method == HTTPMethod::PUT) return updateTemperatures(client);
	else if (path.equals(UPDATE_SENSORS_PATH) && method == HTTPMethod::PUT) return updateSensors(client);

	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_404_NOT_FOUND);
}

/**
	Updates temperatures of all sensors and sends 204 No content response to client when done.

	@param client: Client to which the response is sent
*/
void TemperatureServer::updateTemperatures(EthernetClient& client)
{
	updateTemperatures();
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
}

/**
	Searches for new temperature sensors and sends 204 No content response to client when done.

	@param client: Client to which the response is sent
*/
void TemperatureServer::updateSensors(EthernetClient& client)
{
	updateSensors();
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
}

/**
	Updates temperatures and sends them in JSON-format to client.

	@param client: Client to which the response is sent
*/
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

/**
	Updates temperatures of all sensors.
*/
void TemperatureServer::updateTemperatures()
{
	sensors.requestTemperatures();
}

/**
	Searches for new temperature sensors.
*/
void TemperatureServer::updateSensors()
{
	sensors.begin();
}

/**
	Rounds value to tenths.

	@param value: float value to be rounded
	@return rounded float-value
*/
float TemperatureServer::roundTemp(float value)
{
	return floor(value * 5 + 0.5) / 5;
}

/**
	Get temperature from single temperature sensor.

	@param index: Index of the temperature sensor, index > 0
	@return temperature in float rounded to tenths
*/
float TemperatureServer::getTemperature(int index)
{
	return roundTemp(sensors.getTempCByIndex(index));
}

/**
	Gets temperature sensor ID from single sensor.

	@param index: Index of the temperature sensor, index > 0
	@param outStr: char* to buffer where the ID is stored
*/
void TemperatureServer::getID(int index, char* outStr)
{
	byte address[8];
	sensors.getAddress(address, index);
	array_to_string(address, sizeof(address), outStr);
}

/**
	Converts byte array to char array.

	@param array: byte array to be converted
	@param len: unsigned int number of elements in array
	@param outBuffer: char array where the string representation is stored
*/
void TemperatureServer::array_to_string(byte array[], unsigned int len, char outBuffer[])
{
	for (unsigned int i = 0; i < len; i++) {
		byte nib1 = (array[i] >> 4) & 0x0F;
		byte nib2 = (array[i] >> 0) & 0x0F;
		outBuffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
		outBuffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
	}
	outBuffer[len*2] = '\0';
}
