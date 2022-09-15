#pragma once
#include <opencv2/core.hpp>		// Include OpenCV


class Circle {
public:
	Circle(cv::Vec3f circle, ushort depth) :
		point(circle[0], circle[1]),
		radius(circle[2]),
		depth(depth) { };
	~Circle() {};

	Point point;
	ushort depth;
	float radius;
};