#include "Point.h"
#include "ColorMaps.h"
#include "Consts.h"
#include <functional>

const char *Point::CMAP_NAMES[] = {"Viridis", "Magma", "Inferno", "HSV", "Terrain", "Greyscale"};

static const auto VIRIDIS = colormap::viridis(NUM_COLORS);
static const auto MAGMA = colormap::magma(NUM_COLORS);
static const auto INFERNO = colormap::inferno(NUM_COLORS);
static const auto HSV = colormap::hsv(NUM_COLORS);
static const auto TERRAIN = colormap::terrain(NUM_COLORS);
static const auto GREY = colormap::bone(NUM_COLORS);

std::array<float, 4> Point::getColorFromDepth(CMAP cmap) const 
{
	auto depth = std::clamp(Depth, -1.0f, 1.0f);

	if (depth == -1.0f)
		return { 0.0f };

	auto color_index = (int)((float)NUM_COLORS * (1.0f + depth) / 2.0f);

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

void Point::updateDepth(float depth, float depth_scale, glm::mat4 *intrinsics, CMAP cmap)
{
	this->Depth = isnan(depth) ? -1.0f : (isinf(depth) ? 1.0f : depth / depth_scale);
	std::array<float, 4> Color = getColorFromDepth(cmap);



	Vertices[0].Position[2] = -HalfLength + depth;
	Vertices[0].Color = Color;
	Vertices[1].Position[2] = -HalfLength + depth;
	Vertices[1].Color = Color;
	Vertices[2].Position[2] = HalfLength + depth;
	Vertices[2].Color = Color;
	Vertices[3].Position[2] = HalfLength + depth;
	Vertices[3].Color = Color;

	Vertices[4].Position[2] = -HalfLength + depth;
	Vertices[4].Color = Color;
	Vertices[5].Position[2] = -HalfLength + depth;
	Vertices[5].Color = Color;
	Vertices[6].Position[2] = HalfLength + depth;
	Vertices[6].Color = Color;
	Vertices[7].Position[2] = HalfLength + depth;
	Vertices[7].Color = Color;
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

void Point::updateVertexArray(CMAP cmap)
{
	std::array<float, 4> Color = getColorFromDepth(cmap);

	/*
		7      6
	   .+------+
	4.' |  5 .'|
	+---+--+'  |
	|   |  |   |
	|  .+--+---+2
	|.' 3  | .'Length = 2 * HalfLength
	+------+'
	0	   1
	*/
	Vertices[0] = { { -HalfLength + Position[0], -HalfLength + Position[1], -HalfLength + Depth }, Color };
	Vertices[1] = { {  HalfLength + Position[0], -HalfLength + Position[1], -HalfLength + Depth }, Color };
	Vertices[2] = { {  HalfLength + Position[0], -HalfLength + Position[1],  HalfLength + Depth }, Color };
	Vertices[3] = { { -HalfLength + Position[0], -HalfLength + Position[1],  HalfLength + Depth }, Color };

	Vertices[4] = { { -HalfLength + Position[0],  HalfLength + Position[1], -HalfLength + Depth }, Color };
	Vertices[5] = { {  HalfLength + Position[0],  HalfLength + Position[1], -HalfLength + Depth }, Color };
	Vertices[6] = { {  HalfLength + Position[0],  HalfLength + Position[1],  HalfLength + Depth }, Color };
	Vertices[7] = { { -HalfLength + Position[0],  HalfLength + Position[1],  HalfLength + Depth }, Color };
}
