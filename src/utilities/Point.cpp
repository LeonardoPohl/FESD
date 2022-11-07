#include "Point.h"
#include "ColorMaps.h"
#include "Consts.h"
#include <functional>

void Point::updateVertexArray(float depth, float depth_scale, CMAP cmap)
{
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

	Depth = depth;

	// maybe add cache?
	std::array<float, 4> Color = getColorFromDepth(depth, depth_scale, cmap);

	auto x = (PositionFunction[0] * depth) / Scale;
	auto y = (PositionFunction[1] * depth) / Scale;
	auto z = depth * depth_scale;
	auto a = (HalfLengthFun * depth) / Scale;

	Vertices[0] = { { x - a, y - a, z - a}, Color };
	Vertices[1] = { { x + a, y - a, z - a}, Color };
	Vertices[2] = { { x + a, y - a, z + a}, Color };
	Vertices[3] = { { x - a, y - a, z + a}, Color };

	Vertices[4] = { { x - a, y + a, z - a}, Color };
	Vertices[5] = { { x + a, y + a, z - a}, Color };
	Vertices[6] = { { x + a, y + a, z + a}, Color };
	Vertices[7] = { { x - a, y + a, z + a}, Color };
}

glm::vec3 Point::getPoint()
{
	return { (PositionFunction[0] * Depth), (PositionFunction[1] * Depth),  Depth };
}

const char *Point::CMAP_NAMES[] = { "Viridis", "Magma", "Inferno", "HSV", "Terrain", "Greyscale" };

const auto VIRIDIS = colormap::viridis(NUM_COLORS);
const auto MAGMA = colormap::magma(NUM_COLORS);
const auto INFERNO = colormap::inferno(NUM_COLORS);
const auto HSV = colormap::hsv(NUM_COLORS);
const auto TERRAIN = colormap::terrain(NUM_COLORS);
const auto GREY = colormap::bone(NUM_COLORS);

inline std::array<float, 4> Point::getColorFromDepth(float depth, float depth_scale, CMAP cmap) const
{
	auto z = std::clamp(depth / depth_scale, 0.0f, 1.0f);

	if (z == -1.0f)
		return { 0.0f };

	auto color_index = (int)((float)NUM_COLORS * z);

	if (cmap == CMAP::VIRIDIS)
	{
		auto col = xt::row(VIRIDIS, color_index);
		return { (float)col[0], (float)col[1], (float)col[2], 1.0f };
	}
	else if (cmap == CMAP::MAGMA)
	{
		auto col = xt::row(MAGMA, color_index);
		return { (float)col[0], (float)col[1], (float)col[2], 1.0f };
	}
	else if (cmap == CMAP::INFERNO)
	{
		auto col = xt::row(INFERNO, color_index);
		return { (float)col[0], (float)col[1], (float)col[2], 1.0f };
	}
	else if (cmap == CMAP::TERRAIN)
	{
		auto col = xt::row(TERRAIN, color_index);
		return { (float)col[0], (float)col[1], (float)col[2], 1.0f };
	}
	else if (cmap == CMAP::HSV)
	{
		auto col = xt::row(HSV, color_index);
		return { (float)col[0], (float)col[1], (float)col[2], 1.0f };
	}
	else
	{
		auto col = xt::row(GREY, color_index);
		return { (float)col[0], (float)col[1], (float)col[2], 1.0f };
	}
}

unsigned int *Point::getIndices(int i)
{
	std::array<unsigned int, IndexCount> indices
	{
		0, 1, 2,
		0, 2, 3,
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
