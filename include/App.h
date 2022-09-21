#pragma once

#include <OpenNI.h>
#include <iostream>
#include <vector>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio.hpp>  // Video write

#include <filesystem>
#include <assert.h>

#include <DepthCamera.h>
#include <ImguiBootstrap.h>

int update();

std::vector<DepthCamera*> depthCameras;

int App(std::string_view const &glsl_version) {

    //# initialize imgui
    //##################
    auto window = imgui::create_window(glsl_version);
    if (!window) {
        return 1;
    }

    auto clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGuiIO* io = &ImGui::GetIO();
    io->Fonts->AddFontDefault();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(2);

    //# Camera Initialisation
    //#######################

    auto rs_cameras = RealSenseCamera::initialiseAllDevices();
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices();
    
    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());

    //# Main Loop
    //###########

    auto count = 0;

    imgui::loop(*window, clear_color, update);

    std::cout << std::endl;

    for (DepthCamera* cam : depthCameras) {
        delete cam;
    }
}

int update() {
    cv::Mat frame;
    std::vector<int>::iterator new_end;
    int enabled_cameras = 0;

    for (DepthCamera* cam : depthCameras) {

        ImGui::Begin(cam->getCameraName().c_str());
        ImGui::Checkbox("Enable Camera", &cam->is_enabled);

        if (&cam->is_enabled) {
            ImGui::Checkbox("Detect Spheres", &cam->detect_circles);

            try {
                frame = cam->getFrame();

                //# Sphere Detection
                //##################

                for (Circle const* c : cam->detectSpheres(frame)) {
                    c->drawCircle(frame);
                }

                //# Display window
                //################

                cv::imshow(cam->getWindowName(), frame);
                cv::resizeWindow(cam->getWindowName(), frame.size[1], frame.size[0]);
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

            if (cam->is_enabled) {
                enabled_cameras++;
            }

            if (!frame.empty()) {

            }
        }        

        ImGui::End();
    }
    return enabled_cameras;
}
