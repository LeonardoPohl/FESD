#pragma once
#include <vector>

#include "Point.h"

namespace KDTree 
{
	class Node 
	{
	public:
		Node(int depth);
		~Node();

		void findBestNode(Point* p, Node* currentBest, float bestDistance);

		Point* Median{ };
		Node* LeftChild{ };
		Node* RightChild{ };
		int Depth{ };
	};

	class Tree
	{
	public:
		Tree() {}
		Tree(Point *points[], const int elem_count);

		void updatePointList(Point *points[], const int elem_count);
		void buildTree(Point* points[], const int elem_count, Node* node);
		Point* findNearestNeighbour(Point* point);
	private:
		Node* RootNode{ nullptr };
	};
}


