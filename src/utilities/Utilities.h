#pragma once
#include <random>
#include <iostream>
#include <glm/glm.hpp>

struct BoundingBox
{
public:
	// TODO low prio: draw bounding box
	glm::vec3 minBoundingPoint{};
	glm::vec3 maxBoundingPoint{};

	inline bool updateBox(glm::vec3 p)
	{
		bool isUpdated = false;

		if (p.x > maxBoundingPoint.x){ 
			maxBoundingPoint.x = p.x; 
			isUpdated = true;
		}
		if (p.y > maxBoundingPoint.y){ 
			maxBoundingPoint.y = p.y; 
			isUpdated = true;
		}
		if (p.z > maxBoundingPoint.z){ 
			maxBoundingPoint.z = p.z; 
			isUpdated = true;
		}

		if (p.x < minBoundingPoint.x){ 
			minBoundingPoint.x = p.x; 
			isUpdated = true;
		}
		if (p.y < minBoundingPoint.y){ 
			minBoundingPoint.y = p.y; 
			isUpdated = true;
		}
		if (p.z < minBoundingPoint.z)
		{
			minBoundingPoint.z = p.z; 
			isUpdated = true;
		}

		return isUpdated;
	}
};