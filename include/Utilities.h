#pragma once
#include <opencv2/core/types.hpp>

float calculateDistance(cv::Point3f first, cv::Point3f second) {
	return std::sqrt(std::pow(first.x - second.x, 2) +
					 std::pow(first.y - second.y, 2) +
					 std::pow(first.z - second.z, 2));
}
