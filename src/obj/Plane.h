#pragma once
#include <glm/glm.hpp>

class Plane
{
public:
	Plane(glm::vec3 point, glm::vec3 normal) : point(point), normal(glm::normalize(normal)) { }

	Plane(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) : point(p1) 
	{ 
		normal = glm::normalize(glm::cross((p2 - p1), (p3 - p1)));
	}

	float getDistance(glm::vec3 p) const
	{
		return abs(glm::dot(p, normal) + glm::dot(point, normal));
	}

	bool inDistance(glm::vec3 p, float threshold = 0.0f) const
	{
		return getDistance(p) <= threshold;
	}
private:
	glm::vec3 point;
	glm::vec3 normal;
};

