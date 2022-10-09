#pragma once
#include <opencv2/core.hpp>		// Include OpenCV
#include <opencv2/imgproc.hpp>

class Circle {
public:
	Circle(cv::Vec3f circle, ushort depth, float radius):
		center(cv::Point(circle[0], circle[1])),
		radius(circle[2]),
		depth(depth),
		world_radius(radius) { };
	~Circle() = default;

	void drawCircle(cv::Mat frame) const {
		// circle outline
		cv::circle(frame, center, radius, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
		// circle center
		cv::circle(frame, center, 1, cv::Scalar(0, 100, 100), 3, cv::LINE_AA);
	}

	cv::Point center;
	ushort depth;
	float radius;
	float world_radius;
};
