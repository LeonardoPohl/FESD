#pragma once
#include "Point.h"
#include "BoundingBox.h"

#include <vector>
#include <string>
#include <glm/glm.hpp>

class Cell
{
public:
	Cell(glm::vec3 index, float *m_PlanarThreshold);
	Cell(std::string key, float *m_PlanarThreshold);

	inline void addPoint(Point *p)
	{
		if (p->Depth > 0)
			m_AverageNormal += p->getNormal();
		m_Points.push_back(p);
	}

	bool calculateNDT();

	void updateNDTType();

	glm::vec3 getIndex() const
	{
		return m_Index;
	}

	std::vector<Point*> getPoints() const
	{
		return m_Points;
	}

	glm::vec3 getNormalisedNormal() const
	{
		return glm::normalize(m_AverageNormal);
	}

	bool operator==(Cell other) const 
	{ 
		auto other_index = other.getIndex();
		return m_Index.x == other_index.x && m_Index.y == other_index.y && m_Index.z == other_index.z;
	}

	enum class NDT_TYPE
	{
		Linear,
		Planar,
		Spherical,
		None
	};

	inline NDT_TYPE getType() const
	{
		return m_Type;
	}

	static inline std::string getKey(BoundingBox boundingBox, glm::vec3 cellSize, glm::vec3 point)
	{
		auto coords = (point - boundingBox.getMinPoint()) / cellSize;

		int x = std::round(coords.x);
		int y = std::round(coords.y);
		int z = std::round(coords.z);

		return std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
	}

	static inline glm::vec3 getCellSize(BoundingBox boundingBox, int devisions)
	{
		return (boundingBox.getMaxPoint() - boundingBox.getMinPoint()) / (float)devisions;
	}
private:
	float *m_PlanarThreshold;
	NDT_TYPE m_Type{ NDT_TYPE::None };

	std::vector<Point*> m_Points;
	glm::vec3 m_Index;
	glm::vec3 m_AverageNormal{ 0.0f };
	glm::vec3 m_Covariance{ 0.0f };

	glm::vec3 m_EigenVector{ 0.0f };
};