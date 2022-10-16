#pragma once

#include <array>

class Point
{
public:
	struct Vertex
	{
		std::array<float, 3> Position;
		std::array<float, 4> Color;

		Vertex() :Position{ 0.0f, 0.0f, 0.0f }, Color{ 0.0f, 0.0f, 0.0f, 0.0f } {}
		Vertex(std::array<float, 3> position,
			   std::array<float, 4> color)
			: Position(position), Color(color) {}
	};

	static const int VertexCount = 8;
	static const int IndexCount = 3 * 12;

	std::array<float, 2> Position;
	float Depth{ 0 };
	std::array<Vertex, VertexCount> Vertices;
	float HalfLength;

	Point() : Position{0.0f, 0.0f}{}

	std::array<float, 4> getColorFromDepth();
	void updateDepth(float depth);
	static unsigned int *getIndices(int i);
	void updateVertexArray();
};