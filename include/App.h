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
#include <Parameters.h>

class App {
public:
    // TODO: Implement logger object and print in ImGui
    // TODO: display opencv frame in ImGui I dont like floating frames
    App(std::string_view const& glsl_version);
    static void onMouse(int event, int x, int y, int d, void* ptr)
    {
        if (event == cv::EVENT_LBUTTONDOWN)
        {
            cv::Point* p = (cv::Point*)ptr;
            p->x = x;
            p->y = y;
        }
    }

private:
    void update();
    void initAllCameras();
    std::vector<DepthCamera*> depthCameras;
    ImGuiIO* io;

    Params::GlobalParameters global_params{ &depthCameras };
    Params::SphereDetectionParameters sphere_params{};
    Params::NormalParameters normal_params{};


    GLFWwindow* window;
};