#pragma once
#ifndef POINT
#define POINT

#include <array>
#include <algorithm>

#include <glm/glm.hpp>

#include "Utilities/CMaps.h"

class Point
{
public:
	struct Vertex
	{
		int CameraIndex{ 0 };
		std::array<float, 3> Position;
		std::array<float, 3> Color;
		
		Vertex() : Position{ 0.0f, 0.0f, 0.0f }, Color{ 0.0f, 0.0f, 0.0f }, CameraIndex(0) {}
		Vertex(std::array<float, 3> position, std::array<float, 3> color, int CameraIndex)
			: Position(position), Color(color), CameraIndex(CameraIndex) {}

		inline void reassign(float x, float y, float z, const float r, const float g, const float b) {
			Position[0] = x;
			Position[1] = y;
			Position[2] = z;

			Color[0] = r;
			Color[1] = g;
			Color[2] = b;
		}
	};

	static unsigned int* getIndices(int i) {
		/*
			7      6
		   .+------+
		4.' |  5 .'|
		+---+--+'  |
		|   | p|   |
		|  .+--+---+2
		|.' 3  | .'a
		+------+'
		0	   1
		*/

		std::array<unsigned int, IndexCount> indices
		{
			0, 2, 1,
			0, 3, 2,
			0, 3, 7,
			0, 7, 4,
			1, 0, 5,
			5, 0, 4,
			2, 1, 6,
			6, 1, 5,
			3, 2, 7,
			7, 2, 6,
			5, 4, 6,
			6, 4, 7
		};

		for (int k = 0; k < IndexCount; k++)
			indices[k] += i * VertexCount;

		return &indices[0];
	}

	inline std::array<float, 3> getColorFromDepth(float depth) const {
		float z = std::clamp(depth / 6.0f, 0.0f, 1.0f);
		return CMap::getViridis(z);
	}

	inline void updateVertexArray(float depth, int cam_index = -1)
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

		Vertices[0].reassign(x - a, y - a, z - a, r, g, b);
		Vertices[1].reassign(x + a, y - a, z - a, r, g, b);
		Vertices[2].reassign(x + a, y - a, z + a, r, g, b);
		Vertices[3].reassign(x - a, y - a, z + a, r, g, b);
		
		Vertices[4].reassign(x - a, y + a, z - a, r, g, b);
		Vertices[5].reassign(x + a, y + a, z - a, r, g, b);
		Vertices[6].reassign(x + a, y + a, z + a, r, g, b);
		Vertices[7].reassign(x - a, y + a, z + a, r, g, b);

		if (cam_index != -1)
			for (auto vert : Vertices)
				vert.CameraIndex = cam_index;
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

	static const int VertexCount = 8;
	static const int IndexCount = 3 * 12;

	float Depth{ 0 };
	std::array<Vertex, VertexCount> Vertices;
	std::array<float, 2> PositionFunction{ 0.0f, 0.0f };
	float HalfLengthFun;

	glm::vec3 normal{ 0.f };
};

#endif