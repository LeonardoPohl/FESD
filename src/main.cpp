/// Main.cpp

#include <OpenNI.h>
#include <iostream>
#include <vector>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio.hpp>  // Video write

#include <DepthCamera.h>
#include <ImguiBootstrap.h>

constexpr int NUM_FRAMES = 500;

int update(const std::vector<DepthCamera*> *depthCameras);

int main() {
    //# initialize imgui
    //##################

    auto glsl_version = imgui::glfw_init();
    if (!glsl_version) {
        return 1;
    }

    auto window = imgui::create_window(*glsl_version);
    if (!window) {
        return 1;
    }

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    // Load Fonts
    ImGuiIO* io = &ImGui::GetIO();
    io->Fonts->AddFontDefault();
    ImFont* roboto_font = io->Fonts->AddFontFromFileTTF("./resources/fonts/Roboto-Medium.ttf", 14.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(2);

    //# initialize openni sdk
    //#######################

    if (openni::OpenNI::initialize() != openni::STATUS_OK)
    {
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());
        return 1;
    }

    //# Camera Initialisation
    //#######################

    std::vector<DepthCamera*> depthCameras;

    auto rs_cameras = RealSenseCamera::initialiseAllDevices();
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices();

    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());

    //# Main Loop
    //###########

    auto count = 0;
    while (cv::waitKey(1) < 0 && count < NUM_FRAMES && !depthCameras.empty()) {
        count++;
        std::cout << "\r" << count << " / " << NUM_FRAMES << " Frames (" << 100 * count / NUM_FRAMES << "%)";

        //# Main Settings Frame
        //#####################

        ImGui::PushFont(roboto_font);
        ImGui::Begin("Another Window");

        ImGui::End();



        if (update(&depthCameras) <= 0) {
            std::cout << "All cameras are disabled, terminating now..." << std::endl;
            break;
        }
        std::cout << std::flush;
    }

    std::cout << std::endl;

    for (DepthCamera* cam : depthCameras) {
        delete cam;
    }

    //# Shutdown
    //##########

    std::cout << std::endl;

    openni::OpenNI::shutdown();

    return 0;
}


int update(const std::vector<DepthCamera*> *depthCameras) {
    cv::Mat frame;
    std::vector<int>::iterator new_end;
    int enabled_cameras = 0;

    for (DepthCamera* cam : *depthCameras) {
        try {
            frame = cam->getFrame();
            for (Circle const* c : cam->detectSpheres(frame)) {
                c->drawCircle(frame);
            }

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
    }
    return enabled_cameras;
}
