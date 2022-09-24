#pragma once
#include <opencv2/core/types.hpp>
#include <imgui.h>

float calculateDistance(cv::Point3f first, cv::Point3f second) {
	return std::sqrt(std::pow(first.x - second.x, 2) +
					 std::pow(first.y - second.y, 2) +
					 std::pow(first.z - second.z, 2));
}


void showSphereTable(DepthCamera* cam, cv::Mat depth_frame, cv::Mat edge_frame, SphereDetectionParameters params, bool display_edges) {
    auto spheres = cam->detectSpheres(depth_frame, params);

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

cv::Mat detectEdges(cv::Mat depth_frame, SphereDetectionParameters params) {
    cv::Mat edge_mat = cv::Mat::zeros(depth_frame.size[1], depth_frame.size[0], CV_8UC1);

    depth_frame.convertTo(edge_mat, CV_8UC1);
    cv::adaptiveThreshold(edge_mat, edge_mat, 255, params.adapriveThresholdType, params.thresholdType, 5, 2);
    return edge_mat;
}