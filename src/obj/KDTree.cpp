#include "KDTree.h"
#include <iostream>
#include <future>
#include <thread>

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

Tree::Tree(std::shared_ptr<Point[]> points, const int elem_count) {
	RootNode = new Node{ 0 };
	buildTree(points, elem_count, RootNode);
}


void Tree::updatePointList(std::shared_ptr<Point[]> points, const int elem_count)
{
	delete RootNode;
	RootNode = new Node{ 0 };
	buildTree(points, elem_count, RootNode);
}

void Tree::buildTree(std::shared_ptr<Point[]> points, const int elem_count, Node* node) {
	std::cout << "Building tree at level " << node->Depth << std::endl;
	
	if (elem_count == 0) {
		std::cout << "noooo" << std::endl;
		return;
	}

	if (elem_count == 1) {
		node->Median = points.get();
		return;
	}

	int half_size = elem_count / 2;
	auto comperator = Point::getComparator(node->Depth % 3);
	std::nth_element(points.get(), points.get() + half_size, points.get() + elem_count, comperator);
	node->Median = points.get() + half_size;

	std::vector<Point*> l;
	std::vector<Point*> r;
	
	for (int i = 0; i < elem_count; i++) {
		if ((points.get() + i)->Depth == 0.0f) {
			continue;
		}
		if (comperator(*(points.get() + i), *node->Median)) {
			l.push_back(points.get() + i);
		}
		else {
			r.push_back(points.get() + i);
		}
	}
	
	std::cout << l.size() << " " << half_size << std::endl;
	std::cout << r.size() << " " << elem_count - half_size << std::endl;

	if (node->Depth == 2) {
		std::thread thread_l;
		std::thread thread_r;

		if (!l.empty()) {
			node->LeftChild = new Node(node->Depth + 1);
			std::cout << "Creating Thread for left" << std::endl;
			std::shared_ptr<Point[]> ptr(l[0]);
			thread_l = std::thread(&Tree::buildTree, this, ptr, l.size(), node->LeftChild);
		}

		if (!r.empty()) {
			node->RightChild = new Node(node->Depth + 1);
			std::cout << "Creating Thread for right" << std::endl;
			std::shared_ptr<Point[]> ptr(r[0]);
			thread_r = std::thread(&Tree::buildTree, this, ptr, r.size(), node->RightChild);
		}
		std::cout << "Synchronising right and left" << std::endl;
		thread_l.join();
		thread_r.join();
	}
	else {
		if (!l.empty()) {
			node->LeftChild = new Node(node->Depth + 1);
			std::shared_ptr<Point[]> ptr(l[0]);
			buildTree(ptr, l.size(), node->LeftChild);
		}

		if (!r.empty()) {
			node->RightChild = new Node(node->Depth + 1);
			std::shared_ptr<Point[]> ptr(r[0]);
			buildTree(ptr, r.size(), node->RightChild);
		}
	}
}

Point* Tree::findNearestNeighbour(Point* point)
{
	Node* bestNode = RootNode;
	RootNode->findBestNode(point, bestNode, INFINITY);
	return bestNode->Median;
}
