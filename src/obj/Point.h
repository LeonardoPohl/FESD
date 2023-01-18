#pragma once

#include <array>
#include <algorithm>
#include <functional>

#include <glm/glm.hpp>

#include "Utilities/CMaps.h"

class Point
{
public:
	struct Vertex
	{
		std::array<float, 3> Position;
		std::array<float, 3> Color;
		int CameraIndex{ 0 };
		
		Vertex() : Position{ 0.0f, 0.0f, 0.0f }, Color{ 0.0f, 0.0f, 0.0f }, CameraIndex(0) {}
		Vertex(std::array<float, 3> position, std::array<float, 3> color, int CameraIndex)
			: Position(position), Color(color), CameraIndex(CameraIndex) {}

		inline void reassign(float x, float y, float z, const float r, const float g, const float b, int camId) {
			Position[0] = x;
			Position[1] = y;
			Position[2] = z;

			Color[0] = r;
			Color[1] = g;
			Color[2] = b;

			CameraIndex = camId;
		}
	};

	inline std::array<float, 3> getColorFromDepth(float depth) const {
		float z = std::clamp(depth / 6.0f, 0.0f, 1.0f);
		return CMap::getViridis(z);
	}

	static std::function<bool(Point *p1, Point *p2)> getComparator(int axis) {
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


	static bool compareX(Point* p1, Point* p2)
	{
		return (p1->getPoint().x < p2->getPoint().x);
	}


	static bool compareY(Point* p1, Point* p2)
	{
		return (p1->getPoint().y < p2->getPoint().y);
	}

	static bool compareZ(Point* p1, Point* p2)
	{
		return (p1->getPoint().z < p2->getPoint().z);
	}

	inline void updateVertexArray(float depth, int cam_index)
	{
		Depth = depth;
		std::array<float, 3> Color = getColorFromDepth(depth);
		
		auto x = (PositionFunction[0] * depth);
		auto y = (PositionFunction[1] * depth);
		auto z = depth;
		auto a = (HalfLengthFun * depth);

		auto r = Color[0];
		auto g = Color[1];
		auto b = Color[2];

		Vert.reassign(x, y, z, r, g, b, cam_index);
	}

	inline glm::vec3 getPoint() const
	{
		return { (PositionFunction[0] * Depth), (PositionFunction[1] * Depth),  Depth };
	}

	inline glm::vec3 getNormal() const
	{
		return normal;
	}

	inline glm::vec3 getNormal(glm::vec3 p1, glm::vec3 p2)
	{
		auto p = getPoint();
		normal = glm::normalize(glm::cross((p1 - p), (p2 - p)));
		return normal;
	}


	float Depth{ 0 };
	Vertex Vert;
	std::array<float, 2> PositionFunction{ 0.0f, 0.0f };
	float HalfLengthFun;

	glm::vec3 normal{ 0.f };
};
