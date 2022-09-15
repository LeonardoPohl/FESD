#include "DepthCamera.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>

constexpr auto PI = 3.14159;

using namespace cv;
std::vector<Circle*> DepthCamera::detectSpheres() {
    detectSpheres(this->getFrame());
}
std::vector<Circle*> DepthCamera::detectSpheres(Mat frame) {
    int width = frame.size[0];
    int height = frame.size[1];

    Mat col = Mat::zeros(height, width, IMREAD_COLOR);
    Mat edge_mat = Mat::zeros(height, width, CV_8UC1);

    std::vector<Vec3f> circles;
    frame.convertTo(edge_mat, CV_8UC1);
    adaptiveThreshold(edge_mat, edge_mat, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 2);

    GaussianBlur(edge_mat, edge_mat, Size(9, 9), 2, 2);

    HoughCircles(edge_mat, circles,
        HOUGH_GRADIENT, 1,
        edge_mat.rows / static_cast<double>(16),
        30, 30,
        10, 40);

    cvtColor(edge_mat, col, COLOR_GRAY2BGR);
    std::vector<Circle*> res_circles;

    for (Vec3i c : circles)
    {
        res_circles.push_back(new Circle(c, frame.at<ushort>(c[0], c[1])));
    }

    return res_circles;
}