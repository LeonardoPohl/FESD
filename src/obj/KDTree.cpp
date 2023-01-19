#include "KDTree.h"
#include <iostream>

using namespace KDTree;

Node::Node(int depth) : Depth(depth)
{ }

Node::~Node() {
	delete LeftChild;
	delete RightChild;
}

void KDTree::Node::findBestNode(Point *p, Node* currentBest, float bestDistance)
{
	auto distance = glm::distance(p->getPoint(), Median->getPoint());

	if (distance < bestDistance) {
		bestDistance = distance;
		currentBest = this;
	}

	if (LeftChild != nullptr && RightChild != nullptr) {
		return;
	}

	if (Point::getComparator(Depth % 3)(*p, *Median))
		if (LeftChild != nullptr)
			LeftChild->findBestNode(p, currentBest, bestDistance);
		else
			RightChild->findBestNode(p, currentBest, bestDistance);
	else
		if (RightChild != nullptr)
			RightChild->findBestNode(p, currentBest, bestDistance);
		else
			LeftChild->findBestNode(p, currentBest, bestDistance);
}

Tree::Tree(Point *points[], const int elem_count) {
	RootNode = new Node{ 0 };
	buildTree(points, elem_count, RootNode);
}


void Tree::updatePointList(Point *points[], const int elem_count)
{
	delete RootNode;
	RootNode = new Node{ 0 };
	buildTree(points, elem_count, RootNode);
}

void Tree::buildTree(Point* points[], const int elem_count, Node* node) {
	std::size_t const half_size = elem_count / 2;
	auto comperator = Point::getComparator(node->Depth % 3);
	std::nth_element(*points, *points + half_size, *points + elem_count, comperator);
	node->Median = *points + half_size;

	Point** l = new Point*[half_size];
	Point** r = new Point*[elem_count - half_size];
	int li = 0, ri = 0;
	
	for (int i = 0; i < elem_count; i++) {
		std::cout << (*(points[i])).getPoint().x << " " << (*(points[i])).getPoint().y << " " << (*(points[i])).getPoint().z << std::endl;
		if (comperator(*(points[i]), *node->Median)) {
			l[li] = points[i];
			li += 1;
		}
		else {
			r[ri] = points[i];
			ri += 1;
		}
	}

	if (half_size > 0) {
		node->LeftChild = new Node(node->Depth + 1);
		buildTree(l, half_size, node->LeftChild);
	}

	if (elem_count - half_size > 0) {
		node->RightChild = new Node(node->Depth + 1);
		buildTree(r, elem_count - half_size, node->RightChild);
	}
}
Point* Tree::findNearestNeighbour(Point* point)
{
	Node* bestNode = RootNode;
	RootNode->findBestNode(point, bestNode, INFINITY);
	return bestNode->Median;
}
