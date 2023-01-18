#include "KDTree.h"

using namespace KDTree;

Node::Node(std::vector<Point*> points, int depth) :
	Depth(depth)
{
	std::sort(points.begin(), points.end(), Point::getComparator(depth % 3));

	std::size_t const half_size = points.size() / 2;
	Median = points[half_size];

	std::vector<Point*> l = { points.begin(), points.begin() + half_size };
	std::vector<Point*> r = { points.begin() + half_size, points.begin() };

	if (!l.empty())
		LeftChild  = new Node(l, depth + 1);
	
	if (!r.empty())
		RightChild = new Node(r, depth + 1);
}

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

	if (Point::getComparator(Depth % 3)(p, Median))
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

Point* KDTree::Node::getMedian()
{
	return Median;
}

Tree::Tree(std::vector<Point*> points) {
	RootNode = new Node{ points, 0 };
}

void KDTree::Tree::updatePointList(std::vector<Point*> points)
{
	delete RootNode;
	RootNode = new Node{points, 0};
}

Point* Tree::findNearestNeighbour(Point* point)
{
	Node* bestNode = RootNode;
	RootNode->findBestNode(point, bestNode, INFINITY);
	return bestNode->getMedian();
}
