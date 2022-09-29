#pragma once

#include <iostream>
#include <vector>
#include <filesystem>
#include <future>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio.hpp>  // Video write
#include <opencv2/highgui.hpp>  // Video write

#include <OpenNI.h>

#include <DepthCamera.h>
#include <ImguiBootstrap.h>
#include <SphereDetectionParameters.h>

// TODO: Implement logger object

void update();
void initAllCameras();

SphereDetectionParameters params {};

std::vector<DepthCamera*> depthCameras;
ImGuiIO* io;
float sphere_radius;
bool display_edges = false;
bool walking_average = false;
bool calculate_surface_normals = false;

static void onMouse(int event, int x, int y, int d, void* ptr)
{
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        cv::Point* p = (cv::Point*)ptr;
        p->x = x;
        p->y = y;
    }
    
}


int App(std::string_view const &glsl_version) {

    //# initialize imgui
    //##################

    auto window = imgui::create_window(glsl_version);
    if (!window) {
        return 1;
    }

    auto clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    io = &ImGui::GetIO();
    io->Fonts->AddFontDefault();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(2);

    //# Camera Initialisation
    //#######################

    //std::async(std::launch::async, [] { });
    initAllCameras();
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
    
    for (DepthCamera* cam : depthCameras) {
        cv::namedWindow(cam->getWindowName());
        cv::setMouseCallback(cam->getWindowName(), onMouse, (void*)(&cam->selectedFloorPoint));
    }
}

void update() {
    cv::Mat depth_frame, edge_frame, color_frame, frame, normal_frame, floor_points;
    std::vector<int>::iterator new_end;
    bool reset_walking_frames = false;
    {
        ImGui::Begin("Global Settings");
        ImGui::Text("%d Devices Available", depthCameras.size());

        ImGui::Text("");

        for (DepthCamera* cam : depthCameras) {
            ImGui::SameLine();
            ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);
        }

        params.displayParameters();

        ImGui::Checkbox("Display edges", &display_edges);
        ImGui::Checkbox("Calculate Surface Normals", &calculate_surface_normals);
        
        reset_walking_frames |= ImGui::Checkbox("Use Walking Average", &walking_average);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / io->Framerate, io->Framerate);

        ImGui::End();
    }

    if (reset_walking_frames) {
        for (DepthCamera* cam : depthCameras) {
            cam->walkingFrames.reset();
            cam->walkingEdges.reset();
        }
    }

    for (DepthCamera* cam : depthCameras) {
        if (cam->is_enabled) {
            ImGui::Begin(cam->getCameraName().c_str());
            if (walking_average) {
                if (ImGui::Button("Flush Moving Average")) {
                    cam->walkingFrames.reset();
                    cam->walkingEdges.reset();
                }
                if (ImGui::SliderInt("Moving Average Length", &cam->walkingFrames.length, 1, 100)) {
                    cam->walkingEdges.length = cam->walkingFrames.length;
                }
               
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

                if (cam->show_color_stream) {
                    color_frame = cam->getColorFrame();
                    if (color_frame.rows > 0 && color_frame.cols > 0) {
                        cv::imshow(cam->getWindowName() + " - Color Stream", color_frame);
                        cv::resizeWindow(cam->getWindowName() + " - Color Stream", color_frame.size[1], color_frame.size[0]);
                    }
                }

                //# Display image
                //###############

                if (display_edges) {
                    edge_frame = DepthCamera::detectEdges(depth_frame, params);
                    if (walking_average) {
                        cam->walkingEdges.enqueue(edge_frame);
                        edge_frame = cam->walkingEdges.getValue();
                    }

                    if (edge_frame.rows > 0 && edge_frame.cols > 0) {
                        cv::imshow(cam->getWindowName() + " Edges", edge_frame);
                        cv::resizeWindow(cam->getWindowName() + " Edges", edge_frame.size[1], edge_frame.size[0]);
                    }
                }

                //# Sphere Detection
                //##################

                if (cam->detect_circles) {
                    cam->displaySphereTable(depth_frame, edge_frame, params, display_edges);
                }

                if (cam->selectedFloorPoint.x != -1) {
                    floor_points = cam->calculateSelectedFloor(depth_frame, params);

                    if (floor_points.rows > 0 && floor_points.cols > 0) {
                        cv::imshow(cam->getWindowName() + " Floor", floor_points);
                        cv::resizeWindow(cam->getWindowName() + " Floor", floor_points.size[1], floor_points.size[0]);
                    }
                }
                if (depth_frame.rows > 0 && depth_frame.cols > 0) {
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
            ImGui::End();
        }
    }
}
