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

	static const int VertexCount = 8;
	static const int IndexCount = 3 * 12;

	std::array<float, 2> Position{ 0.0f, 0.0f };
	std::array<float, 2> PositionFunction{ 0.0f, 0.0f };
	float Depth{ 0 };
	std::array<Vertex, VertexCount> Vertices;
	float HalfLength;

	static const int CMAP_COUNT = 6;
	static const char * CMAP_NAMES[];
	
	static enum class CMAP
	{
		VIRIDIS,
		MAGMA,
		INFERNO,
		HSV,
		TERRAIN,
		GREY
	};

	std::array<float, 4> getColorFromDepth(CMAP cmap) const;
	void updateDepth(float depth, float depth_scale, CMAP cmap = CMAP::VIRIDIS);
	static unsigned int *getIndices(int i);
	void updateVertexArray(CMAP cmap = CMAP::VIRIDIS);
};