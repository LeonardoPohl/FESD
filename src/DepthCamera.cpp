#include "DepthCamera.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <Utilities.h>

using namespace cv;

std::vector<Circle*> DepthCamera::detectSpheres(SphereDetectionParameters params) {
    return detectSpheres(this->getDepthFrame(), params);
}

std::vector<Circle*> DepthCamera::detectSpheres(Mat frame, SphereDetectionParameters params) {
    if (!this->detect_circles) {
        return std::vector<Circle*>();
    }

    int width = frame.size[0];
    int height = frame.size[1];

    Mat col = Mat::zeros(height, width, IMREAD_COLOR);
    Mat edge_mat = Mat::zeros(height, width, CV_8UC1);

    std::vector<Vec3f> circles;
    frame.convertTo(edge_mat, CV_8UC1);

    if (params.simple_edge_detection) {
        adaptiveThreshold(edge_mat, edge_mat, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 5, 2);
    }
    else {
        for (int w = 1; w < width-1; w++) {
            for (int h = 1; h < height-1; h++) {
                if (std::abs(frame.at<unsigned char>(w, h) - frame.at<unsigned char>(w - 1, h - 1)) > params.edge_depth_diff ||
                    std::abs(frame.at<unsigned char>(w, h) - frame.at<unsigned char>(w - 1, h + 0)) > params.edge_depth_diff ||
                    std::abs(frame.at<unsigned char>(w, h) - frame.at<unsigned char>(w - 1, h + 1)) > params.edge_depth_diff ||
                    std::abs(frame.at<unsigned char>(w, h) - frame.at<unsigned char>(w + 0, h - 1)) > params.edge_depth_diff ||
                    std::abs(frame.at<unsigned char>(w, h) - frame.at<unsigned char>(w + 0, h + 1)) > params.edge_depth_diff ||
                    std::abs(frame.at<unsigned char>(w, h) - frame.at<unsigned char>(w + 1, h - 1)) > params.edge_depth_diff ||
                    std::abs(frame.at<unsigned char>(w, h) - frame.at<unsigned char>(w + 1, h + 0)) > params.edge_depth_diff ||
                    std::abs(frame.at<unsigned char>(w, h) - frame.at<unsigned char>(w + 1, h + 1)) > params.edge_depth_diff) {
                    edge_mat.at<unsigned char>(w, h) = 255;
                }
            }
        }
    }


    cv::GaussianBlur(edge_mat, edge_mat, Size(9, 9), 2, 2);

    int hough_method = HOUGH_GRADIENT;
    double min_dist = edge_mat.rows / static_cast<double>(16);

    cv::HoughCircles(edge_mat, circles,
        hough_method, 1,
        min_dist,
        params.param1, params.param2,
        params.min_radius, params.max_radius);

    cv::cvtColor(edge_mat, col, COLOR_GRAY2BGR);
    std::vector<Circle*> res_circles;

    for (Vec3i c : circles)
    {
        if (frame.at<ushort>(c[1], c[0]) > 0 && frame.at<ushort>(c[1], c[0]) < 500000) {
            auto A = this->pixelToPoint(c[1], c[0], frame.at<ushort>(c[1], c[0]));
            auto B = this->pixelToPoint(c[1], c[0], frame.at<ushort>(c[1], c[0] + c[2] * 0.9));
            auto dist = calculateDistance(A, B);
            if (dist < params.sphere_radius) {
                res_circles.push_back(new Circle(c, frame.at<ushort>(c[1], c[0]), dist));
            }
        }
    }

    return res_circles;
}