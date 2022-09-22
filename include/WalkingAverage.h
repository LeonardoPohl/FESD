#pragma once
#include <opencv2/core/types.hpp>
#include <queue>

class WalkingAverageMatrix {
public:
	WalkingAverageMatrix() : length(500) {}
	WalkingAverageMatrix(int size) : length(size) {}

	cv::Mat getValue() {
		std::queue<cv::Mat> q_copy = q;
		cv::Mat val = cv::Mat();
		while (!q_copy.empty())
		{
			if (val.empty()) {
				val = q_copy.front() / q.size();
			}
			else {
				val += q_copy.front() / q.size();
			}

			q_copy.pop();
		}

		return val;
	}
	
	void reset() {
		std::queue<cv::Mat> empty;
		std::swap(q, empty);
	}

	void enqueue(cv::Mat mat) {
		while (q.size() >= length) {
			q.pop();
		}
		q.push(mat);
	}

	std::queue<cv::Mat> q;
	int length;
};