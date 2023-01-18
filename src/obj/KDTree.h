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

		void findBestNode(Point* p, Node* currentBest, float bestDistance);
		Point* getMedian();
	private:
		Point* Median{ };
		Node* LeftChild{ };
		Node* RightChild{ };
		int Depth;
	};

	class Tree
	{
	public:
		Tree() {}
		Tree(std::vector<Point*> points);

		void updatePointList(std::vector<Point*> points);
		Point* findNearestNeighbour(Point* point);
	private:
		std::vector<Point*> Points;
		Node* RootNode{ nullptr };
	};
}


