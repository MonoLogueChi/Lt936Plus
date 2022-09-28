// AdcFilter.h

#ifndef _ADCFILTER_h
#define _ADCFILTER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


class AdcFilter
{
public:
	AdcFilter(uint16_t A, uint16_t N);
	AdcFilter();
	int readMiliVolts(uint8_t pin, int oldValue);
private:
	uint16_t a;
	uint16_t n;
};

#endif

