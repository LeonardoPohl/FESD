#include "DepthCamera.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.inl.hpp>
#include <utilities/Utilities.h>
#include <numbers>
#include <algorithm>
#include <iostream>
#include <parameters/Parameters.h>

using namespace cv;

std::vector<Circle*> DepthCamera::detectSpheres(Params::SphereDetectionParameters *params) {
    return detectSpheres(this->getDepthFrame(), params);
}

std::vector<Circle*> DepthCamera::detectSpheres(Mat frame, Params::SphereDetectionParameters *params) {
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
        params->param1, params->param2,
        params->min_radius, params->max_radius);

    cvtColor(edge_mat, col, COLOR_GRAY2BGR);
    std::vector<Circle*> res_circles;

    for (Vec3i c : circles)
    {
        if (frame.at<ushort>(c[1], c[0]) > 0 && frame.at<ushort>(c[1], c[0]) < 500000) {
            auto A = this->pixelToPoint(c[1], c[0], frame.at<ushort>(c[1], c[0]));
            auto B = this->pixelToPoint(c[1], c[0], frame.at<ushort>(c[1], c[0] + c[2] * 0.9));
            auto dist = calculateDistance(A, B);
            if (dist < params->sphere_radius) {
                res_circles.push_back(new Circle(c, frame.at<ushort>(c[1], c[0]), dist));
            }
        }
    }

    return res_circles;
}

void DepthCamera::displaySphereTable(cv::Mat depth_frame, cv::Mat edge_frame, Params::SphereDetectionParameters *params, bool display_edges) {
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


cv::Mat DepthCamera::detectEdges(cv::Mat depth_frame, Params::SphereDetectionParameters *params) {
    cv::Mat edge_mat = cv::Mat::zeros(depth_frame.size[1], depth_frame.size[0], CV_8UC1);
    if (depth_frame.empty()) {
        return edge_mat;
    }
    depth_frame.convertTo(edge_mat, CV_8UC1);
    cv::adaptiveThreshold(edge_mat, edge_mat, 255, params->adapriveThresholdType, params->thresholdType, 5, 2);
    return edge_mat;
}

cv::Mat DepthCamera::getWorldFrame(cv::Mat depth_frame)
{
    Mat world_frame(depth_frame.size(), CV_32FC3);
    for (int x = 1; x < depth_frame.rows; ++x)
    {
        for (int y = 1; y < depth_frame.cols; ++y)
        {
            if (depth_frame.at<ushort>(x, y) == 0) {
                continue;
            }

            world_frame.at<Vec3f>(x, y) = this->pixelToPoint(x, y, depth_frame.at<ushort>(x, y));
        }
    }
    return cv::Mat();
}

cv::Mat DepthCamera::calculateSelectedFloor(cv::Mat depth_frame, Params::NormalParameters *params)
{
    if (this->selectedFloorPoint.x == -1 || depth_frame.empty()) {
       // return cv::Mat();
    }
    int width = depth_frame.size().width;
    int height = depth_frame.size().height;
    Mat normals(height - 2, width - 2, CV_32FC3);
    Mat floor(depth_frame.size(), CV_8UC3);

    for (int row = 2; row < depth_frame.rows; ++row)
    {
        for (int col = 2; col < depth_frame.cols; ++col)
        {
            if ((depth_frame.at<ushort>(row, col) == 0) ||
                (depth_frame.at<ushort>(row - 1, col) == 0) ||
                (depth_frame.at<ushort>(row, col - 1) == 0)) {
                continue;
            }

            if ((depth_frame.at<ushort>(row, col) == depth_frame.at<ushort>(row, col - 1)) ||
                (depth_frame.at<ushort>(row, col) == depth_frame.at<ushort>(row - 1, col))) {
                continue;
            }

            Vec3d t(row, col - 1, depth_frame.at<ushort>(row - 1, col)/*depth(y-1,x)*/);
            Vec3d l(row - 1, col, depth_frame.at<ushort>(row, col - 1)/*depth(y,x-1)*/);
            Vec3d c(row, col, depth_frame.at<ushort>(row, col)/*depth(y,x)*/);

            Vec3d d = (l - c).cross(t - c);
            Vec3d n = normalize(d);
            /*
            float dzdx = depth_frame.at<ushort>(row + 1, col) - depth_frame.at<ushort>(row - 1, col);
            float dzdy = depth_frame.at<ushort>(row, col + 1) - depth_frame.at<ushort>(row, col - 1);
            
            Vec3f d(-dzdx,
                    -dzdy,
                    1);

            Vec3f n = normalize(d);*/
            float edge_falloff = 1;

            edge_falloff -= std::abs((float)depth_frame.at<ushort>(row, col) - depth_frame.at<ushort>(row + 1, col + 1));
            edge_falloff -= std::abs((float)depth_frame.at<ushort>(row, col) - depth_frame.at<ushort>(row + 0, col + 1));
            edge_falloff -= std::abs((float)depth_frame.at<ushort>(row, col) - depth_frame.at<ushort>(row - 1, col + 1));
            edge_falloff -= std::abs((float)depth_frame.at<ushort>(row, col) - depth_frame.at<ushort>(row + 1, col + 0));
            edge_falloff -= std::abs((float)depth_frame.at<ushort>(row, col) - depth_frame.at<ushort>(row - 1, col + 0));
            edge_falloff -= std::abs((float)depth_frame.at<ushort>(row, col) - depth_frame.at<ushort>(row + 1, col - 1));
            edge_falloff -= std::abs((float)depth_frame.at<ushort>(row, col) - depth_frame.at<ushort>(row + 0, col - 1));
            edge_falloff -= std::abs((float)depth_frame.at<ushort>(row, col) - depth_frame.at<ushort>(row - 1, col - 1));

            edge_falloff *= 1 / params->edgeCutoff;
            edge_falloff = std::max(edge_falloff, 0.01f);
            
            n[0] = 255 * edge_falloff * ((n[0] + 1) / 2);
            n[1] = 255 * edge_falloff * ((n[1] + 1) / 2);
            n[2] = 255 * edge_falloff * ((n[2] + 1) / 2);

            normals.at<Vec3f>(row - 2, col - 2) = n;

        }
    }
    if (params->blur) {
        GaussianBlur(normals, normals, Size(9, 9), 2, 2);
    }
    return normals;
    
    // TODO: Detect sphere positioned above ground as selected floor point

    floodFill(normals, floor, this->selectedFloorPoint, 255);

    Vec3f avg_normal;
    float floor_normals = 0;

    for (int x = 1; x < floor.rows; ++x)
    {
        for (int y = 1; y < floor.cols; ++y)
        {
            if (floor.at<int>(x, y) == 1) {
                floor_normals += 1;
                float edge_falloff = normals.at<Vec3f>(x, y)[0] + normals.at<Vec3f>(x, y)[0] + normals.at<Vec3f>(x, y)[2];
                avg_normal[0] += normals.at<Vec3f>(x, y)[0] * edge_falloff;
                avg_normal[1] += normals.at<Vec3f>(x, y)[1] * edge_falloff;
                avg_normal[2] += normals.at<Vec3f>(x, y)[2] * edge_falloff;
            }
        }
    }

    avg_normal[0] /= floor_normals;
    avg_normal[1] /= floor_normals;
    avg_normal[2] /= floor_normals;

    this->floorNormal = avg_normal;
    std::cout << avg_normal << std::endl;
    std::cout << floor_normals << std::endl;
    Mat col = Mat::zeros(normals.rows, normals.cols, IMREAD_COLOR);
    cvtColor(depth_frame, col, COLOR_GRAY2BGR);

    for (int x = 1; x < floor.rows; ++x)
    {
        for (int y = 1; y < floor.cols; ++y)
        {
            if (floor.at<int>(x, y) == 1) {
                col.at<Scalar>(x, y) = avg_normal;
            }
        }
    }

    return normals;
}
