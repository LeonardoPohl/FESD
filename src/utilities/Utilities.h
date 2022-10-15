#pragma once
#include <random>
#include <iostream>

float Normalisem11(float val01)
{
	if (val01 > 1.0f || val01 < 0.0f)
	{
		std::cout << "[WARNING] " << val01 << " not in correct range (0..1)" << std::endl;
	}
	return (val01 * 2.0f) - 1.0f;
}

float Normalise01(float valm11)
{
	if (valm11 > 1.0f || valm11 < -1.0f)
	{
		std::cout << "[WARNING] " << valm11 << " not in correct range (0..1)" << std::endl;
	}
	return (valm11 + 1.0f) / 2.0f;
}