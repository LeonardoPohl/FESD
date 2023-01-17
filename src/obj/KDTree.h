#pragma once
#include <vector>

#include "Point.h"

namespace KDTree 
{
	class Node 
	{
	public:
		Node(std::vector<Point*> points, int depth);
		~Node();

		void matchPoint(Point* p, Node* currentBest);
	private:
		glm::vec3 Median{ };
		Node* LeftChild{ };
		Node* RightChild{ };
		int Depth;
	};

	class Tree
	{
		Tree() {}
		Tree(std::vector<Point*> points);

		void updatePointList(std::vector<Point*> points);
		glm::vec3 findNearestNeighbour(Point* point);
	private:
		std::vector<Point*> Points;
		Node* RootNode{ nullptr };
	};
}


