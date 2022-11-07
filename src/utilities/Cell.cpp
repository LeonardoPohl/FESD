#include "Cell.h"

Cell::Cell(glm::vec3 index, float m_PlanarThreshold) : m_PlanarThreshold(m_PlanarThreshold), m_Index(index) { }

/// <summary>
/// Calculates the Cell Parameters
/// </summary>
/// <returns>True if λ1/λ2 ≤ te</returns>
bool Cell::calculateNDT()
{
	//m_AverageNormal = getNormalisedNormal();

	// Covariance Matrix
	// ⎡   var(x), cov(x,y), cov(x,z) ⎤
	// ⎢ cov(x,y),   var(y), cov(y,z) ⎥
	// ⎣ cov(x,z), cov(y,z),   var(z) ⎦
}