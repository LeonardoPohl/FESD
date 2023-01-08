#pragma once
#include <random>
#include <glm/glm.hpp>

class BoundingBox
{
public:
	inline bool updateBox(glm::vec3 p)
	{
		bool isUpdated = false;

		if (p.x > m_MaxBoundingPoint.x){ 
			m_MaxBoundingPoint.x = p.x; 
			isUpdated = true;
		}
		if (p.y > m_MaxBoundingPoint.y){ 
			m_MaxBoundingPoint.y = p.y;
			isUpdated = true;
		}
		if (p.z > m_MaxBoundingPoint.z){ 
			m_MaxBoundingPoint.z = p.z; 
			isUpdated = true;
		}

		if (p.x < m_MinBoundingPoint.x){ 
			m_MinBoundingPoint.x = p.x; 
			isUpdated = true;
		}
		if (p.y < m_MinBoundingPoint.y){ 
			m_MinBoundingPoint.y = p.y; 
			isUpdated = true;
		}
		if (p.z < m_MinBoundingPoint.z)
		{
			m_MinBoundingPoint.z = p.z; 
			isUpdated = true;
		}

		return isUpdated;
	}

	glm::vec3 getMinPoint() {
		return m_MinBoundingPoint;
	}

	glm::vec3 getMaxPoint() {
		return m_MaxBoundingPoint;
	}
private:
	glm::vec3 m_MinBoundingPoint{};
	glm::vec3 m_MaxBoundingPoint{};
};