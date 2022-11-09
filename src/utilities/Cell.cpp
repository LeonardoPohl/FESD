#include "Cell.h"

#include <numeric>
#include <cmath>

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
	assert(a->size() == b->size());

	auto mean_a = std::reduce(a->begin(), a->end()) / a->size();
	auto mean_b = std::reduce(b->begin(), b->end()) / b->size();

	float covariance = 0;

	for (int i = 0; i < a->size(); i++)
	{
		covariance += ((*a)[i] - mean_a) * ((*b)[i] - mean_b);
	}

	return covariance / a->size();
}


float calcSymmetricalDeterminant(glm::mat3x3 A)
{
	return A[0][0] * A[1][1] * A[2][2]
		 + A[0][1] * A[1][2] * A[0][2]
		 + A[0][2] * A[0][1] * A[1][2]
		  
		 - A[0][2] * A[1][1] * A[0][2]
		 - A[0][1] * A[0][1] * A[2][2]
		 - A[0][0] * A[1][2] * A[1][2];
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

	for (auto pixel : m_Points)
	{
		auto p = pixel->getPoint();
		x.push_back(p.x);
		y.push_back(p.y);
		z.push_back(p.z);
	}

	// Covariance Matrix
	glm::mat3x3 covariance = { cov(&x, &x), cov(&x, &y), cov(&x, &z),
							   cov(&x, &y), cov(&y, &y), cov(&y, &z),
							   cov(&x, &z), cov(&y, &z), cov(&z, &z) };

	glm::mat3x3 identity = { 1, 0, 0,
							 0, 1, 0,
							 0, 0, 1 };

	auto trace = covariance[0][0] + covariance[1][1] + covariance[2][2];
	
	auto p1 = glm::pow(covariance[1][2], 2) + glm::pow(covariance[1][3], 2) + glm::pow(covariance[2][3], 2);
	if (p1 == 0)
	{
		// Covariance is diagonal
		m_EigenVector = { covariance[0][0], covariance[1][1], covariance[2][2]};
	}
	else
	{
		auto q = trace / 3;
		auto p2 = glm::pow(covariance[0][0] - q, 2) + glm::pow(covariance[1][1], 2) + glm::pow(covariance[2][2], 2) + 2 * p1;
		auto p = glm::sqrt(p2 / 6);
		auto B = (float)(1.0f / p) * (covariance - q * identity);

		auto r = calcSymmetricalDeterminant(covariance);

		// In exact arithmetic for a symmetric matrix - 1 <= r <= 1
		// but computation error can leave it slightly outside this range.
		double phi;
		if (r <= -1)
			phi = std::_Pi / 3.0;
		else if(r >= 1)
			phi = 0.0;
		else
			phi = acos(r) / 3.0;

		auto eig1 = q + 2.0 * p * cos(phi);
		auto eig3 = q + 2.0 * p * cos(phi + (2.0 * std::_Pi / 3.0));
		auto eig2 = 3.0 * q - eig1 - eig3; // since trace(A) = eig1 + eig2 + eig3;

		m_EigenVector = { eig1, eig2, eig3 };
	}

	if (m_EigenVector.y == 0)
		return false;

	return m_EigenVector.x / m_EigenVector.y <= m_PlanarThreshold;
}