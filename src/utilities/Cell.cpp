#include "Cell.h"

Cell::Cell(glm::vec3 index, float m_PlanarThreshold) : m_Index(index), m_PlanarThreshold(m_PlanarThreshold) { }

/// <summary>
/// Calculates the Cell Parameters
/// </summary>
/// <returns>True if λ1/λ2 ≤ te</returns>
bool Cell::calculateNDT()
{
	glm::vec3 normal;
	
	for (auto p : m_Points)
	{
		normal += p->getNormal();
	}

	m_AverageNormal = glm::normalize(normal);

	// Covariance Matrix
	// ⎡   var(x), cov(x,y), cov(x,z) ⎤
	// ⎢ cov(x,y),   var(y), cov(y,z) ⎥
	// ⎣ cov(x,z), cov(y,z),   var(z) ⎦
}