#define ARDUINOJSON_ENABLE_PROGMEM 1
#include <ArduinoJson.h>
#include "FanServer.hpp"
#include "HTTP.hpp"

#define JSON_BUFFER_SIZE 350
#define PIN_PARAMETER F("pin")
#define FREQUENCY_PARAMETER F("frequency")
#define DUTYCYCLE_PARAMETER F("dutycycle")
#define CONFIG_PARAMETER F("config")
#define LIMITS_ATTRIBUTE F("limits")
#define DEFAULTS_ATTRIBUTE F("defaults")
#define FANPINS_ATTRIBUTE F("fanpins")


FanServer::FanServer()
: _fans(), _fanCount(0)
{
	InitTimersSafe();
}

FanServer::~FanServer()
{
	for (Fan fan : _fans) {
		fan.~Fan();
	}
}

/**
	Handles HTTP-request. If does not recognize request path and/or method, sends 404 Not found.

	@param request: First line of a HTTP-request
	@param client: Client to which the response is sent
*/
void FanServer::handleRequest(const String& request, EthernetClient& client)
{
	String path = HTTP::parseRequestPath(request, 2);
	HTTPMethod method = HTTP::getRequestMethod(request);

	if (method == HTTPMethod::PUT) {
		return setFanProperties(client, request);
	} else if (method == HTTPMethod::POST && path.length() == 0) {
		return addFan(client, request);
	} else if (method == HTTPMethod::DELETE && path.length() == 0) {
		return removeFan(client, request);
	} else if (method == HTTPMethod::GET) {
		if (path.length() == 0) return sendFansJson(client, request);
		if (path.equals(CONFIG_PARAMETER)) return sendConfigJson(client);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_404_NOT_FOUND);
}

/**
	Adds new fan to _fans and sends 201 Created response to the client with JSON
	body. If fan cannot be added, sends 400 Bad request response to the client.
	Pin-parameter is mandatory, frequency and dutycycle are optional.
	Default values are defined in Fan.hpp.

	@param client: Client to which the response is sent
	@param request: First line of a HTTP-request
*/
void FanServer::addFan(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	int frequency = HTTP::parseRequestParameterIntValue(request, FREQUENCY_PARAMETER);
	int dutyCycle = HTTP::parseRequestParameterIntValue(request, DUTYCYCLE_PARAMETER);

	// If fan doesn't exists and adding new fan fails, send error message and return
	if (!findFan(pin) && !addFan(pin)) {
		HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
		return;
	}

	setFrequency(pin, frequency);
	setDutyCycle(pin, dutyCycle);

	sendSingleFan(client, pin, HTTPResponseType::HTTP_201_CREATED);
}

/**
	Removes fan from _fans and sends 204 No content response to the client.
	If specified fan is not found or cannot be removed, sends 400 Bad request response to the client.

	@param client: Client to which the response is sent
	@param request: First line of a HTTP-request
*/
void FanServer::removeFan(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	if (removeFan(pin)) {
		return HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
}

/**
	Sends fan-information of all fans in JSON format to the client.

	@param client: Client to which the response is sent
	@param request: First line of a HTTP-request
*/
void FanServer::sendFansJson(EthernetClient &client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);

	if (pin > 0) {
		sendSingleFan(client, pin, HTTPResponseType::HTTP_200_OK);
	} else {
		StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
		JsonArray& root = jsonBuffer.createArray();

		for (int i=0; i<_fanCount; i++) {
			addFanInfoToJsonObj(_fans[i], root.createNestedObject());
		}

		HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_200_OK);
		root.printTo(client);
	}
}

/**
	Sends fan-information in JSON format to the client.
	If fan is not found, sends HTTP 400 Bad Request.

	@param client: Client to which the response is sent
	@param responseType: Response-code sent to client if fan is found
	@param pin: Pin number of the fan
*/
void FanServer::sendSingleFan(EthernetClient& client, int pin, HTTPResponseType responseType)
{
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;

	Fan* fan = findFan(pin);
	if (fan) {
		JsonObject& fanObj = jsonBuffer.createObject();
		addFanInfoToJsonObj(*fan, fanObj);

		HTTP::sendHttpResponse(client, responseType);
		fanObj.printTo(client);
	} else {
		HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
	}
}

/**
	Sends all config information: defaults, limits, minimum and maximum value
	for fans in JSON format to the client.

	@param client: Client to which the response is sent
*/
void FanServer::sendConfigJson(EthernetClient &client)
{
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();

	JsonObject& limits = root.createNestedObject(LIMITS_ATTRIBUTE);
	JsonObject& defaults = root.createNestedObject(DEFAULTS_ATTRIBUTE);
	JsonArray& fanPins = root.createNestedArray(FANPINS_ATTRIBUTE);

	defaults[F("dutycycle")] = DEFAULT_DUTYCYCLE;
	defaults[F("frequency")] = DEFAULT_FREQUENCY;

	limits[F("min dutycycle")] = MIN_DUTYCYCLE;
	limits[F("min frequency")] = MIN_FREQUENCY;
	limits[F("max frequency")] = MAX_FREQUENCY;

	for (int pin : _allowedFanPins) {
		fanPins.add(pin);
	}

	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_200_OK);
	root.printTo(client);
}

/**
	Sets dutycycle to a fan and sends 204 No content response to the client.
	Pin number and new dutycycle must be specified in the request.
	If dutycycle cannot be applied, 400 Bad request is sent to the client.

	@param client: Client to which the response is sent
	@param request: First line of a HTTP-request
*/
void FanServer::setFanProperties(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);

	if (findIndex(pin) < 0) {
		HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
		return;
	}

	int dutyCycle = HTTP::parseRequestParameterIntValue(request, DUTYCYCLE_PARAMETER);
	int frequency = HTTP::parseRequestParameterIntValue(request, FREQUENCY_PARAMETER);

	setDutyCycle(pin, dutyCycle);
	setFrequency(pin, frequency);

	sendSingleFan(client, pin, HTTPResponseType::HTTP_200_OK);
}

/**
	Finds index of a fan in _fans.

	@param pin: number of a fan pin
	@return Index of o pin. If no fan is found, returns -1.
*/
int FanServer::findIndex(int pin)
{
	for (int i=0; i<_fanCount; i++) {
		if (_fans[i].getPin() == pin) {
			return i;
		}
	}
	return -1;
}

/**
	Finds fan from _fans.

	@param pin: nmumber of a fan pin
	@return Fan*, if no fan is found returns nullptr.
*/
Fan* FanServer::findFan(int pin)
{
	int index = findIndex(pin);
	if (index >= 0) {
		return &_fans[index];
	}
	return nullptr;
}

/**
	Checks if fan pin is already in use.

	@param pin: Pin number to be checked
	@return True if fan pin is free to use, otherwise False
*/
bool FanServer::isfreePin(int pin)
{
	for (int allowedPin : _allowedFanPins) {
		if (pin == allowedPin) {
			for (int i=0; i<_fanCount; i++) {
				if (_fans[i].getPin() == pin) return false;
			}
			return true;
		}
	}
	return false;
}

/**
	Adds fan information to JsonObject.

	@param fan: desired fan to extract the information from
	@param outFanJsonObject: Object where the fan information is stored
*/
void FanServer::addFanInfoToJsonObj(Fan& fan, JsonObject &outFanJsonObject)
{
	outFanJsonObject[PIN_PARAMETER] = fan.getPin();
	outFanJsonObject[FREQUENCY_PARAMETER] = fan.getFrequency();
	outFanJsonObject[DUTYCYCLE_PARAMETER] = fan.getDutycycle();
}

/**
	Creates new fan and adds it to _fans.

	@param pin: Pin number of the fan
	@param frequency: Frequency to the new fan, Min and Max frequency specified in Fan.hpp
	@param dutycycle: dutyCycle to the new fan, 0 <= dutyCycle <= 100
	@return True if addition was successful, otherwise False.
*/
bool FanServer::addFan(int pin, int frequency, int dutyCycle)
{
	if (_fanCount < MAX_FAN_COUNT && isfreePin(pin) && SetPinFrequencySafe(pin, frequency)) {
		_fans[_fanCount] = Fan(pin, frequency, dutyCycle);
		_fans[_fanCount].init();
		_fanCount++;
		return true;
	}
	return false;
}

/**
	Removes fan from _fans and calls the destructor.

	@param pin: Pin number from fan to be removed
	@return True if removing the fan was succesful, otherwise False
*/
bool FanServer::removeFan(int pin)
{
	int index = findIndex(pin);

	if (index >= 0) {
		_fans[index].~Fan();
		//Check if removing last fan of array
		if (!(index == _fanCount-1)) {
			//Move every fan backwards one index
			for (int i=index; i<_fanCount; i++) {
				_fans[i] = _fans [i + 1];
			}
		}
		_fanCount--;
		return true;
	}
	return false;
}

/**
	Sets new frequency to fan specified.

	@param pin: Pin number of a fan
	@param frequency: new frequency
	@return True if the frequency was successfully changed.
		If not succesful or no fan found, returns false.
*/
bool FanServer::setFrequency(int pin, int frequency)
{
	Fan* fan = findFan(pin);
	if (fan) {
		return fan->setFrequency(frequency);
	}
	return false;
}

/**
	Sets new dutycycle to fan specified.

	@param pin: Pin number of a fan
	@param dutyCycle: new dutyCycle
	@return True if the dutycycle was successfully changed.
		If not succesful or no fan found, returns false.
*/
bool FanServer::setDutyCycle(int pin, int dutyCycle)
{
	Fan* fan = findFan(pin);
	if (fan) {
		return fan->setDutyCycle(dutyCycle);
	}
	return false;
}
