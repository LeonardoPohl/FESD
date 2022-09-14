#pragma once
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#define PI 3.14159

using namespace cv;

cv::Mat detectionSpheres(cv::Mat depth_image) {
    int width = depth_image.size[0];
    int height = depth_image.size[1];

    Mat col = Mat::zeros(height, width, IMREAD_COLOR);
    Mat edge_mat = Mat::zeros(height, width, CV_8UC1); 

    std::vector<Vec3f> circles;
    depth_image.convertTo(edge_mat, CV_8UC1);
    adaptiveThreshold(edge_mat, edge_mat, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 2);

    GaussianBlur(edge_mat, edge_mat, Size(9, 9), 2, 2);

    HoughCircles(edge_mat, circles,
        HOUGH_GRADIENT, 1,
        edge_mat.rows / static_cast<double>(16),
        30, 30,
        10, 40);

    cvtColor(edge_mat, col, COLOR_GRAY2BGR);

    for (size_t i = 0; i < circles.size(); i++)
    {
        Vec3i c = circles[i];
        Point center = Point(c[0], c[1]);
        int radius = c[2];

        // circle outline
        circle(col, center, radius, Scalar(0, 255, 0), FILLED, LINE_AA);
        // circle center
        circle(col, center, 1, Scalar(0, 100, 100), 3, LINE_AA);
    }

    return col;
}