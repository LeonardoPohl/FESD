#pragma once

#include <array>
#include <algorithm>
#include <functional>

#include <glm/glm.hpp>

#include "Utilities/CMaps.h"

class Point
{
public:
	Point() : PositionFunction{ 0.0f }, Depth(0.0f), Color{ 0.0f }, CamId( 0.0f ) {}

	std::array<float, 3> getColorFromDepth(float depth) const {
		float z = std::clamp(depth / 6.0f, 0.0f, 1.0f);
		return CMap::getViridis(z);
	}

	static std::function<bool(Point p1, Point p2)> getComparator(int axis) {
		if (axis == 0) {
			return compareX;
		}
		else if (axis == 1) {
			return compareY;
		}
		else if (axis == 2) {
			return compareZ;
		}
		return compareX;
	}

	static bool compareX(Point p1, Point p2)
	{
		return (p1.getPoint().x < p2.getPoint().x);
	}


	static bool compareY(Point p1, Point p2)
	{
		return (p1.getPoint().y < p2.getPoint().y);
	}

	static bool compareZ(Point p1, Point p2)
	{
		return (p1.getPoint().z < p2.getPoint().z);
	}

	void updateVertexArray(float depth, int cam_index)
	{
		Color = getColorFromDepth(depth);
		Depth = depth;
		CamId = cam_index;
	}

	glm::vec3 getPoint() const
	{
		return { PositionFunction[0] * Depth,  PositionFunction[1] * Depth,  Depth};
	}

	glm::vec3 getNormal(glm::vec3 p1, glm::vec3 p2)
	{
		auto p = getPoint();
		return glm::normalize(glm::cross((p1 - p), (p2 - p)));
	}

	std::array<float, 2> PositionFunction{ 0.0f, 0.0f };
	float Depth{ 0.0f };
	std::array<float, 3> Color{ };
	int CamId{ 0 };
};
