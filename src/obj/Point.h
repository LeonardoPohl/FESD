#pragma once

#include <array>
#include <glm/glm.hpp>

class Point
{
public:
	struct Vertex
	{
		std::array<float, 3> Position;
		std::array<float, 4> Color;

		Vertex() : Position{ 0.0f, 0.0f, 0.0f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f } {}
		Vertex(std::array<float, 3> position,
			   std::array<float, 4> color)
			: Position(position), Color(color) {}
	};
		
	enum class CMAP
	{
		VIRIDIS,
		MAGMA,
		INFERNO,
		HSV,
		TERRAIN,
		GREY
	};

	std::array<float, 4> getColorFromDepth(float depth, CMAP cmap) const;
	static unsigned int *getIndices(int i);
	void updateVertexArray(float depth, CMAP cmap = CMAP::VIRIDIS);

	inline glm::vec3 getPoint() const
	{
		return { (PositionFunction[0] * Depth), (PositionFunction[1] * Depth),  Depth };
	}

	inline glm::vec3 getNormal() const
	{
		return normal;
	}

	glm::vec3 calculateNormal(glm::vec3 p1, glm::vec3 p2)
	{
		auto p = getPoint();
		normal = glm::normalize(glm::cross((p1 - p), (p2 - p)));
		return normal;
	}

	static const int VertexCount = 8;
	static const int IndexCount = 3 * 12;

	std::array<float, 2> PositionFunction{ 0.0f, 0.0f };
	float Depth{ 0 };
	std::array<Vertex, VertexCount> Vertices;
	float HalfLengthFun;

	glm::vec3 normal{ 0.f };

	static const int CMAP_COUNT = 6;
	static const char *CMAP_NAMES[];
};