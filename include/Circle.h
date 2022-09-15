#pragma once
#include <opencv2/core.hpp>		// Include OpenCV


class Circle {
public:
	Circle(cv::Vec3f circle) :
		point(circle[0], circle[1]),
		radius(circle[2]) {	};
	~Circle() {};

	Point point;
	float radius;
}