#pragma once
#include <opencv2/core/types.hpp>
#include <random>

float calculateDistance(cv::Point3f first, cv::Point3f second) {
	return std::sqrt(std::pow(first.x - second.x, 2) +
					 std::pow(first.y - second.y, 2) +
					 std::pow(first.z - second.z, 2));
}

// See https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
std::vector<int> FisherYatesShuffle(std::size_t size, std::size_t max_size, std::mt19937& gen)
{
    assert(size <= max_size);
    std::vector<int> b(size);

    for (std::size_t i = 0; i != max_size; ++i) {
        std::uniform_int_distribution<> dis(0, i);
        std::size_t j = dis(gen);
        if (j < b.size()) {
            if (i < b.size()) {
                b[i] = b[j];
            }
            b[j] = i;
        }
    }
    return b;
}
