#define ARDUINOJSON_ENABLE_PROGMEM 1
#include <ArduinoJson.h>
#include "FanServer.hpp"
#include "HTTP.hpp"

#define JSON_BUFFER_SIZE 350
#define PIN_PARAMETER F("pin")
#define FREQUENCY_PARAMETER F("frequency")
#define DUTYCYCLE_PARAMETER F("dutycycle")
#define FANS_ATTRIBUTE F("fans")
#define PIN_ATTRIBUTE F("pins")
#define FREE_PIN_ATTRIBUTE F("freepins")
#define FANPINS_ATTRIBUTE F("fanpins")
#define DEFAULTS_PARAMETER F("defaults")


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
		if (request.indexOf(FREQUENCY_PARAMETER) != -1) return setFrequency(client, request);
		if (request.indexOf(DUTYCYCLE_PARAMETER) != -1) return setDutyCycle(client, request);
	} else if (method == HTTPMethod::POST && path.length() == 0) {
		return addFan(client, request);
	} else if (method == HTTPMethod::DELETE && path.length() == 0) {
		return removeFan(client, request);
	} else if (method == HTTPMethod::GET) {
		if (path.length() == 0) return sendFansJson(client, request);
		if (path.equals(FREE_PIN_ATTRIBUTE)) return sendPinsJson(client);
		if (path.equals(FANPINS_ATTRIBUTE)) return sendPinsJson(client, false);
		if (path.equals(DEFAULTS_PARAMETER)) return sendDefaultsJson(client);
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

	if (addFan(pin)) {
		setFrequency(pin, frequency);
		setDutyCycle(pin, dutyCycle);
		HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_201_CREATED);
		StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();
		JsonArray& data = root.createNestedArray(F("data"));
		addFanInfoToJsonObj((*(findFan(pin))), data.createNestedObject());
		root.printTo(client);
	} else {
		HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
	}
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
	Sends fan-information in JSON format to the client.

	@param client: Client to which the response is sent
	@param request: First line of a HTTP-request
*/
void FanServer::sendFansJson(EthernetClient &client, const String& request)
{
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	JsonObject& data = root.createNestedObject((F("data")));

	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	if (pin > 0) {
		//Single fan
		Fan* fan = findFan(pin);
		if (fan) addFanInfoToJsonObj(*fan, data.createNestedObject(F("fan")));
		else return HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
	} else {
		JsonArray& fans = data.createNestedArray(FANS_ATTRIBUTE);
		//All the fans
		for (int i=0; i<_fanCount; i++) {
			addFanInfoToJsonObj(_fans[i], fans.createNestedObject());
		}
	}

	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_200_OK);
	root.printTo(client);
}

/**
	Sends list of pins in JSON format to the client. FreePinsMode sends all
	fan pins, otherwise only sends fan pins that are not in use.
	Default is freepins mode.

	@param client: Client to which the response is sent
	@param freePinsMode: Boolean that switches between freepins and fanpin modes,
		default = true
*/
void FanServer::sendPinsJson(EthernetClient &client, bool freePinsMode)
{
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	JsonObject& data = root.createNestedObject((F("data")));
	JsonArray& freePins = data.createNestedArray(PIN_ATTRIBUTE);

	for (int pin : _allowedFanPins) {
		if (freePinsMode && !isfreePin(pin)) continue;
		freePins.add(pin);
	}

	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_200_OK);
	root.printTo(client);
}

/**
	Sends default, minimum and maximum values for fans in JSON format to the client.

	@param client: Client to which the response is sent
*/
void FanServer::sendDefaultsJson(EthernetClient &client)
{
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	JsonObject& data = root.createNestedObject((F("data")));
	JsonObject& defaults = data.createNestedObject(DEFAULTS_PARAMETER);
	defaults[F("dutycycle")] = DEFAULT_DUTYCYCLE;
	defaults[F("frecuency")] = DEFAULT_FREQUENCY;
	defaults[F("min dutycycle")] = MIN_DUTYCYCLE;
	defaults[F("min frequency")] = MIN_FREQUENCY;
	defaults[F("max frequency")] = MAX_FREQUENCY;

	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_200_OK);
	root.printTo(client);
}

/**
	Sets frequency to a fan and sends 204 No content response to the client.
	Pin number and new frequency must be specified in the request.
	If frequency cannot be applied, 400 Bad request is sent to the client.

	@param client: Client to which the response is sent
	@param request: First line of a HTTP-request
*/
void FanServer::setFrequency(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	int frequency = HTTP::parseRequestParameterIntValue(request, FREQUENCY_PARAMETER);
	if (setFrequency(pin, frequency)) {
		return HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
}

/**
	Sets dutycycle to a fan and sends 204 No content response to the client.
	Pin number and new dutycycle must be specified in the request.
	If dutycycle cannot be applied, 400 Bad request is sent to the client.

	@param client: Client to which the response is sent
	@param request: First line of a HTTP-request
*/
void FanServer::setDutyCycle(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	int dutyCycle = HTTP::parseRequestParameterIntValue(request, DUTYCYCLE_PARAMETER);

	if (setDutyCycle(pin, dutyCycle)) {
		return HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
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
