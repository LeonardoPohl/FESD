#include "Cell.h"

Cell::Cell(glm::vec3 index, float m_PlanarThreshold) : m_PlanarThreshold(m_PlanarThreshold), m_Index(index) { }
Cell::Cell(std::string key, float m_PlanarThreshold) : m_PlanarThreshold(m_PlanarThreshold) 
{
	int x = stoi(key.substr(0, key.find(",")));
	key.erase(0, key.find(",") + 1);
	int y = stoi(key.substr(0, key.find(",")));
	key.erase(0, key.find(",") + 1);
	int z = stoi(key.substr(0, key.find(",")));

	m_Index = { x, y, z };
}

/// <summary>
/// 
/// </summary>
/// <returns>Covariance of the inserted values</returns>
float cov(const std::vector<float> *a, const std::vector<float> *b)
{

}

/// <summary>
/// Calculates the Cell Parameters
/// </summary>
/// <returns>True if λ1/λ2 ≤ te</returns>
bool Cell::calculateNDT()
{
	std::vector<float> x;
	std::vector<float> y;
	std::vector<float> z;

	for (auto p : m_Points)
	{
		x.push_back(p->getPoint().x);
		y.push_back(p->getPoint().y);
		z.push_back(p->getPoint().z);
	}

	// Covariance Matrix
	// ⎡   var(x), cov(x,y), cov(x,z) ⎤
	// ⎢ cov(x,y),   var(y), cov(y,z) ⎥
	// ⎣ cov(x,z), cov(y,z),   var(z) ⎦

	glm::mat3x3 covariance = { cov(&x, &x), cov(&x, &y), cov(&x, &z),
							   cov(&x, &y), cov(&y, &y), cov(&y, &z),
							   cov(&x, &z), cov(&y, &z), cov(&z, &z) };
	
}