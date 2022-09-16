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
	[[nodiscard]] uint16_t GetValue(uint8_t pin, uint16_t oldValue) const;
private:
	uint16_t _a;
	uint16_t _n;
};

#endif

