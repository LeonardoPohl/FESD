#include "KDTree.h"

using namespace KDTree;

Node::Node(std::vector<Point*> points, int depth) :
	Depth(depth)
{
	if (points.empty()) {
		return;
	}

	std::sort(points.begin(), points.end(), Point::getComparator(depth % 3));

	std::size_t const half_size = points.size() / 2;
	Median = points[half_size]->getPoint();

	LeftChild  = new Node({ points.begin(), points.begin() + half_size }, depth + 1);
	RightChild = new Node({ points.begin() + half_size, points.begin() }, depth + 1);
}

void KDTree::Node::matchPoint(Point *p, Node* currentBest)
{
	if (LeftChild)
	LeftChild->matchPoint(p, currentBest);
	RightChild->matchPoint(p, currentBest);
}

Node::~Node() {
	delete LeftChild;
	delete RightChild;
}

Tree::Tree(std::vector<Point*> points) {
	RootNode = new Node{ points, 0 };
}

void KDTree::Tree::updatePointList(std::vector<Point*> points)
{
	delete RootNode;
	RootNode = new Node{points, 0};
}

glm::vec3 Tree::findNearestNeighbour(Point* point)
{
	return glm::vec3();
}
