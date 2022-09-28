// 
// 
// 

#include "AdcFilter.h"

int adc_k = 0;
int adc_k0 = 0;

AdcFilter::AdcFilter(uint16_t A, uint16_t N)
{
	a = A;
	n = N;
}

AdcFilter::AdcFilter()
{
	a = 30;
	n = 8;
}

int AdcFilter::readMiliVolts(uint8_t pin, int oldValue)
{
	adc_k = 0;
	for (int i = 0; i < n; i++)
	{
		adc_k += analogReadMilliVolts(pin);
	}

	adc_k0 = adc_k / n;

	if (abs(adc_k0 - oldValue) > a)
	{
		return adc_k0;
	}
	return oldValue;
}



