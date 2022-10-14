#pragma once
#include <opencv2/core/types.hpp>
#include <random>
#include <iostream>

float calculateDistance(cv::Point3f first, cv::Point3f second) {
	return std::sqrt(std::pow(first.x - second.x, 2) +
					 std::pow(first.y - second.y, 2) +
					 std::pow(first.z - second.z, 2));
}

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