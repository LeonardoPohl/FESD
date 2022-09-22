#pragma once

#include <OpenNI.h>

#include <iostream>
#include <vector>
#include <filesystem>
#include <future>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio.hpp>  // Video write

#include <DepthCamera.h>
#include <ImguiBootstrap.h>

void update();
void initAllCameras();

SphereDetectionParameters params;

std::vector<DepthCamera*> depthCameras;
ImGuiTableFlags sphereTableFlags = ImGuiTableFlags_Resizable + ImGuiTableFlags_Borders;
ImGuiIO* io;
float sphere_radius;
bool display_edges = false;
bool walking_average = true;

int App(std::string_view const &glsl_version) {

    //# initialize imgui
    //##################

    auto window = imgui::create_window(glsl_version);
    if (!window) {
        return 1;
    }

    auto clear_color = ImVec4(0.0f, 0.0f, 0.3f, 1.00f);

    io = &ImGui::GetIO();
    io->Fonts->AddFontDefault();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(2);

    //# Camera Initialisation
    //#######################

    std::async(std::launch::async, [] { initAllCameras(); });

    //# Main Loop
    //###########

    auto count = 0;

    imgui::loop(*window, clear_color, update);
    
    std::cout << std::endl;

    for (DepthCamera* cam : depthCameras) {
        delete cam;
    }
}
void initAllCameras() {
    auto rs_cameras = RealSenseCamera::initialiseAllDevices();
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices();

    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());
}

void update() {
    cv::Mat depth_frame, color_frame, frame;
    std::vector<int>::iterator new_end;
    
    {
        ImGui::Begin("Global Settings");
        ImGui::Text("%d Devices Available", depthCameras.size());

        ImGui::SliderFloat("Sphere Radius", &params.sphere_radius, 0, 100);
        ImGui::SliderInt("Min Circle Radius", &params.min_radius, 0, 100);
        ImGui::SliderInt("Max Circle Radius", &params.max_radius, 0, 100);
        ImGui::SliderFloat("Canny edge detector threshold", &params.param1, 0, 100);
        ImGui::SliderFloat("Accumulator threshold", &params.param2, 0, 100);

        ImGui::Checkbox("Display edges", &display_edges);
        ImGui::Checkbox("Use Walking Average", &walking_average);
        ImGui::Checkbox("Simple Edge Detection", &params.simple_edge_detection);
        ImGui::SliderFloat("Edge Difference Threshold", &params.edge_depth_diff, 10, 1000);
        

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / io->Framerate, io->Framerate);

        ImGui::End();
    }

    for (DepthCamera* cam : depthCameras) {
        ImGui::Begin(cam->getCameraName().c_str());
        ImGui::Checkbox("Enable Camera", &cam->is_enabled);

        if (cam->is_enabled) {
            if (ImGui::Button("Flush Moving Average")) {
                cam->walkingFrames.reset();
            }
            ImGui::Checkbox("Detect Spheres", &cam->detect_circles);

            if (cam->hasColorStream()) {
                ImGui::Checkbox("Color Stream", &cam->show_color_stream);
            }
            else if (cam->show_color_stream) {
                cam->show_color_stream = false;
            }
            
            try {
                depth_frame = cam->getDepthFrame();
                if (walking_average) {
                    cam->walkingFrames.enqueue(depth_frame);
                    depth_frame = cam->walkingFrames.getValue();
                }
                //# Sphere Detection
                //##################

                if (cam->detect_circles) {
                    auto spheres = cam->detectSpheres(depth_frame, params);

                    ImGui::Text("Detected Spheres: %d", spheres.size());
                    ImGui::BeginTable("Spheres", 5, sphereTableFlags);

                    ImGui::TableSetupColumn("");
                    ImGui::TableSetupColumn("Radius");
                    ImGui::TableSetupColumn("World Radius");
                    ImGui::TableSetupColumn("Position");
                    ImGui::TableSetupColumn("Distance");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < spheres.size(); i++) {
                        spheres[i]->drawCircle(depth_frame);
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

                if (cam->show_color_stream) {
                    color_frame = cam->getColorFrame();
                    cv::imshow(cam->getWindowName() + " - Color Stream", color_frame);
                    cv::resizeWindow(cam->getWindowName() + " - Color Stream", color_frame.size[1], color_frame.size[0]);
                }

                //# Display image
                //###############

                if (display_edges) {
                    cv::Mat edge_mat = cv::Mat::zeros(depth_frame.size[1], depth_frame.size[0], CV_8UC1);

                    depth_frame.convertTo(edge_mat, CV_8UC1);
                    if (&params.simple_edge_detection) {
                        cv::adaptiveThreshold(edge_mat, edge_mat, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 5, 2);
                    }
                    else {
                        for (int w = 1; w < edge_mat.size[1] - 1; w++) {
                            for (int h = 1; h < edge_mat.size[0] - 1; h++) {
                                if (std::abs(depth_frame.at<unsigned char>(w, h) - depth_frame.at<unsigned char>(w - 1, h - 1)) > params.edge_depth_diff ||
                                    std::abs(depth_frame.at<unsigned char>(w, h) - depth_frame.at<unsigned char>(w - 1, h + 0)) > params.edge_depth_diff ||
                                    std::abs(depth_frame.at<unsigned char>(w, h) - depth_frame.at<unsigned char>(w - 1, h + 1)) > params.edge_depth_diff ||
                                    std::abs(depth_frame.at<unsigned char>(w, h) - depth_frame.at<unsigned char>(w + 0, h - 1)) > params.edge_depth_diff ||
                                    std::abs(depth_frame.at<unsigned char>(w, h) - depth_frame.at<unsigned char>(w + 0, h + 1)) > params.edge_depth_diff ||
                                    std::abs(depth_frame.at<unsigned char>(w, h) - depth_frame.at<unsigned char>(w + 1, h - 1)) > params.edge_depth_diff ||
                                    std::abs(depth_frame.at<unsigned char>(w, h) - depth_frame.at<unsigned char>(w + 1, h + 0)) > params.edge_depth_diff ||
                                    std::abs(depth_frame.at<unsigned char>(w, h) - depth_frame.at<unsigned char>(w + 1, h + 1)) > params.edge_depth_diff) {
                                    edge_mat.at<unsigned char>(w, h) = 255;
                                }
                            }
                        }
                    }

                    cv::imshow(cam->getWindowName(), edge_mat);
                    cv::resizeWindow(cam->getWindowName(), edge_mat.size[1], edge_mat.size[0]);
                }
                else {
                    cv::imshow(cam->getWindowName(), depth_frame);
                    cv::resizeWindow(cam->getWindowName(), depth_frame.size[1], depth_frame.size[0]);
                }
            }
            catch (cv::Exception e) {
                std::cout << " | " << e.msg;
                if (cam->detect_circles) {
                    cam->detect_circles = false;
                }
                else {
                    cam->is_enabled = false;
                }
            }
            catch (std::exception e) {
                std::cout << " | " << e.what();
                if (cam->detect_circles) {
                    cam->detect_circles = false;
                }
                else {
                    cam->is_enabled = false;
                }
            }
        }

        ImGui::End();
    }
}
