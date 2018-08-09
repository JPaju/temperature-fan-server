#define ARDUINOJSON_ENABLE_PROGMEM 1
#include <ArduinoJson.h>
#include "FanServer.hpp"
#include "HTTP.hpp"

#define JSON_BUFFER_SIZE 350
#define PIN_PARAMETER F("pin")
#define FREQUENCY_PARAMETER F("frequency")
#define DUTYCYCLE_PARAMETER F("dutycycle")
#define FANS_ATTRIBUTE F("fans")
#define ALLOWED_PIN_ATTRIBUTE F("allowedpins")


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

void FanServer::handleRequest(const String& request, EthernetClient& client)
{
	String path = HTTP::parseRequestPath(request, 2);
	HTTPMethod method = HTTP::getRequestMethod(request);

	if (method == HTTPMethod::PUT) {
		if (request.indexOf(DUTYCYCLE_PARAMETER) != -1) return setDutyCycle(client, request);
		if (request.indexOf(FREQUENCY_PARAMETER) != -1) return setFrequency(client, request);
	} else if (method == HTTPMethod::POST && path.length() == 0) {
		return addFan(client, request);
	} else if (method == HTTPMethod::DELETE && path.length() == 0) {
		return removeFan(client, request);
	} else if (method == HTTPMethod::GET && path.length() == 0) {
		return getFans(client);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_404_NOT_FOUND);
}

void FanServer::addFan(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	int frequency = HTTP::parseRequestParameterIntValue(request, FREQUENCY_PARAMETER);
	int dutyCycle = HTTP::parseRequestParameterIntValue(request, DUTYCYCLE_PARAMETER);

	if (isAllowedPin(pin) && addFan(pin)) {
		setFrequency(pin, frequency);
		setDutyCycle(pin, dutyCycle);
		HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
	} else {
		HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
	}
}

void FanServer::removeFan(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	if (removeFan(pin)) {
		return HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
}

void FanServer::getFans(EthernetClient &client)
{
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();

	JsonArray& allowedPins = root.createNestedArray(ALLOWED_PIN_ATTRIBUTE);
	for (int i=0; i<MAX_FAN_COUNT; i++) {
		allowedPins.add(_allowedFanPins[i]);
	}

	JsonArray& fans = root.createNestedArray(FANS_ATTRIBUTE);
	for (int i=0; i<_fanCount; i++) {
		JsonObject& fan = fans.createNestedObject();
		fan[PIN_PARAMETER] = _fans[i].getPin();
		fan[FREQUENCY_PARAMETER] = _fans[i].getFrequency();
		fan[FREQUENCY_PARAMETER] = _fans[i].getDutycycle();
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_200_OK);
	root.prettyPrintTo(client);
}

void FanServer::setFrequency(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	int frequency = HTTP::parseRequestParameterIntValue(request, FREQUENCY_PARAMETER);
	if (setFrequency(pin, frequency)) {
		return HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
}

void FanServer::setDutyCycle(EthernetClient& client, const String& request)
{
	int pin = HTTP::parseRequestParameterIntValue(request, PIN_PARAMETER);
	int dutyCycle = HTTP::parseRequestParameterIntValue(request, DUTYCYCLE_PARAMETER);

	if (setDutyCycle(pin, dutyCycle)) {
		return HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_204_NO_CONTENT);
	}
	HTTP::sendHttpResponse(client, HTTPResponseType::HTTP_400_BAD_REQUEST);
}

int FanServer::findIndex(int pin)
{
	for (int i=0; i<_fanCount; i++) {
		if (_fans[i].getPin() == pin) {
			return i;
		}
	}
	return -1;
}

Fan* FanServer::findFan(int pin)
{
	int index = findIndex(pin);
	if (index >= 0) {
		return &_fans[index];
	}
	return nullptr;
}

bool FanServer::isAllowedPin(int pin)
{
	for (int i=0; i<MAX_FAN_COUNT; i++) {
		if (_allowedFanPins[i] == pin) return true;
	}
	return false;
}

bool FanServer::addFan(int pin, int frequency, int dutyCycle)
{
	if (_fanCount < MAX_FAN_COUNT && isAllowedPin(pin) && SetPinFrequencySafe(pin, frequency)) {
		_fans[_fanCount] = Fan(pin, frequency, dutyCycle);
		_fans[_fanCount].init();
		if (findFan(pin) == 0) _fanCount++;
		return true;
	}
	return false;
}

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

bool FanServer::setFrequency(int pin, int frequency)
{
	Fan* fan = findFan(pin);
	if (fan) {
		return fan->setFrequency(frequency);
	}
	return false;
}

bool FanServer::setDutyCycle(int pin, int dutyCycle)
{
	Fan* fan = findFan(pin);
	if (fan) {
		return fan->setDutyCycle(dutyCycle);
	}
	return false;
}
