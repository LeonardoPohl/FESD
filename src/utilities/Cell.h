#pragma once
#include "Point.h"

#include <vector>
#include <string>
#include <glm/glm.hpp>

class Cell
{
public:
	Cell(glm::vec3 index, float m_PlanarThreshold = 0.04f);

	inline void addPoint(Point p)
	{
		m_Points.push_back(p);
	}

	bool calculateNDT();

	glm::vec3 getIndex() const
	{
		return m_Index;
	}

	std::vector<Point> getPoints() const
	{
		return m_Points;
	}

	bool operator==(Cell other) const 
	{ 
		auto other_index = other.getIndex();
		return m_Index.x == other_index.x && m_Index.y == other_index.y && m_Index.z == other_index.z;
	}
private:
	float m_PlanarThreshold;

	std::vector<Point> m_Points;
	glm::vec3 m_Index;
	glm::vec3 m_AverageNormal{ 0.0f };
	glm::vec3 m_Covariance{ 0.0f };
};