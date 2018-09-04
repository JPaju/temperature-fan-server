#include "Fan.hpp"

Fan::Fan(){}

Fan::Fan(int pin, int frequency, int dutyCycle)
: _pin(pin), _frequency(frequency), _dutyCycle(dutyCycle)
{
}

Fan::~Fan()
{
	setDutyCycle(0);
}

/**
	Initalizes fan pin frequency and dutycycle.
*/
void Fan::init()
{
	setFrequency(_frequency);
	setDutyCycle(_dutyCycle);
}

/**
	Sets new frequency. Checks validity of the new frequency first.

	@param frequency: new frequency
	@return True if the frequency was successfully changed, otherwise false.
*/
bool Fan::setFrequency(int frequency)
{
	if (frequency <= MAX_FREQUENCY && SetPinFrequencySafe(_pin, frequency)) {
		_frequency = frequency;
		return true;
	}
	return false;
}

/**
	Sets new dutycycle. Checks validity of the new dutyucycle first.
	If 0 < dutyCycle < MIN_DUTYCYCLE, _dutyCycle = MIN_DUTYCYCLE.


	@param dutycycle: new dutycycle
	@return True if the dutycycle was successfully changed, otherwise false.
*/
bool Fan::setDutyCycle(int dutyCycle)
{
	if (dutyCycle < 101 && dutyCycle > -1) {
		if (dutyCycle == 0) {
			pwmWrite(_pin, 0);
		} else if (dutyCycle < MIN_DUTYCYCLE) {
			pwmWrite(_pin, map(MIN_DUTYCYCLE, 0, 100, 0, 255));
			dutyCycle = MIN_DUTYCYCLE;
		} else {
			pwmWrite(_pin, map(dutyCycle, 0, 100, 0, 255));
		}
		_dutyCycle = dutyCycle;
		return true;
	}
	return false;
}

int Fan::getPin()
{
	return _pin;
}

int Fan::getFrequency()
{
	return _frequency;
}

int Fan::getDutycycle()
{
	return _dutyCycle;
}
