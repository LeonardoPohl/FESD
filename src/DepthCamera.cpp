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
    Mat edge_mat = DepthCamera::detectEdges(frame, params);

    std::vector<Vec3f> circles;

    GaussianBlur(edge_mat, edge_mat, Size(9, 9), 2, 2);

    int hough_method = HOUGH_GRADIENT;
    double min_dist = edge_mat.rows / static_cast<double>(16);

    HoughCircles(edge_mat, circles,
        hough_method, 1,
        min_dist,
        params.param1, params.param2,
        params.min_radius, params.max_radius);

    cvtColor(edge_mat, col, COLOR_GRAY2BGR);
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

void DepthCamera::displaySphereTable(cv::Mat depth_frame, cv::Mat edge_frame, SphereDetectionParameters params, bool display_edges) {
    auto spheres = this->detectSpheres(depth_frame, params);

    ImGui::Text("Detected Spheres: %d", spheres.size());
    ImGui::BeginTable("Spheres", 5, ImGuiTableFlags_Resizable + ImGuiTableFlags_Borders);

    ImGui::TableSetupColumn("");
    ImGui::TableSetupColumn("Radius");
    ImGui::TableSetupColumn("World Radius");
    ImGui::TableSetupColumn("Position");
    ImGui::TableSetupColumn("Distance");
    ImGui::TableHeadersRow();

    for (int i = 0; i < spheres.size(); i++) {
        spheres[i]->drawCircle(depth_frame);
        if (display_edges) {
            spheres[i]->drawCircle(edge_frame);
        }
        if (i < 5) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", i);
            ImGui::TableNextColumn();
            ImGui::Text("%f", spheres[i]->radius);
            ImGui::TableNextColumn();
            ImGui::Text("%f", spheres[i]->world_radius);
            ImGui::TableNextColumn();
            ImGui::Text("%d, %d", spheres[i]->center.x, spheres[i]->center.y);
            ImGui::TableNextColumn();
            ImGui::Text("%d", spheres[i]->depth);
        }
    }

    ImGui::EndTable();
}


cv::Mat DepthCamera::detectEdges(cv::Mat depth_frame, SphereDetectionParameters params) {
    cv::Mat edge_mat = cv::Mat::zeros(depth_frame.size[1], depth_frame.size[0], CV_8UC1);
    if (depth_frame.empty()) {
        return edge_mat;
    }
    depth_frame.convertTo(edge_mat, CV_8UC1);
    cv::adaptiveThreshold(edge_mat, edge_mat, 255, params.adapriveThresholdType, params.thresholdType, 5, 2);
    return edge_mat;
}

cv::Mat DepthCamera::calculateSurfaceNormals(cv::Mat depth_frame, SphereDetectionParameters params)
{
    Mat normals(depth_frame.size(), CV_32FC3);
    if (depth_frame.empty()) {
        return normals;
    }
    for (int x = 1; x < depth_frame.rows; ++x)
    {
        for (int y = 1; y < depth_frame.cols; ++y)
        {
            float dzdx = (depth_frame.at<ushort>(x + 1, y) - depth_frame.at<ushort>(x - 1, y)) / 2.0;
            float dzdy = (depth_frame.at<ushort>(x, y + 1) - depth_frame.at<ushort>(x, y - 1)) / 2.0;

            if ((depth_frame.at<ushort>(x, y) == 0) || (dzdx == 0 && dzdy == 0)) {
                continue;
            }

            Vec3f d(-dzdx, -dzdy, 1.0f);
            Vec3f n = normalize(d);

            if (std::acos(n[0]) < 0.1f) {
                normals.at<Vec3f>(x, y) = n;
            }

        }
    }


    // Assume that up is up around
    // Filter everything thats not pointing upwartds

    return normals;
}
