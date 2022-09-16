// 
// 
// 

#include "AdcFilter.h"

uint32_t k = 0;
uint16_t k0 = 0;

AdcFilter::AdcFilter(uint16_t A, uint16_t N)
{
	_a = A;
	_n = N;
}

AdcFilter::AdcFilter()
{
	_a = 30;
	_n = 8;
}

uint16_t AdcFilter::GetValue(uint8_t pin, uint16_t oldValue) const
{
	k = 0;
	for (int i = 0; i < _n; i++)
	{
		k += analogRead(pin);
	}

	k0 = k / _n;

	if (abs(k0 - oldValue) > _a)
	{
		return k0;
	}
	return oldValue;
}



