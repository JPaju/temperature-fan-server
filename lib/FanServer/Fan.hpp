#ifndef Fan_h
#define Fan_h

#define MAX_FREQUENCY 32767
#define MIN_FREQUENCY 20
#define MIN_DUTYCYCLE 15
#define DEFAULT_FREQUENCY 25000
#define DEFAULT_DUTYCYCLE 15

#include "PWM.h"

class Fan {
private:
	int _pin;
	int _frequency;
	int _dutyCycle;

public:

	Fan();
	Fan(int pin, int frequency, int dutyCycle);
	~Fan();

	void init();
	bool setFrequency(int frequency);
	bool setDutyCycle(int dutyCycle);

	int getPin();
	int getFrequency();
	int getDutycycle();
};

#endif
