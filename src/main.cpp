/// Main.cpp

#include <OpenNI.h>
#include <iostream>
#include <vector>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio.hpp>  // Video write

#include "DepthCamera.h"

constexpr int NUM_FRAMES = 500;

void shut_down(const std::vector<DepthCamera*>* depthCameras);

int main() {
    //initialize openni sdk and rs context
    openni::Status rc = openni::OpenNI::initialize();
    if (rc != openni::STATUS_OK)
    {
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());
        return 1;
    }

    rs2::context ctx;

    // Get devices

    openni::Array<openni::DeviceInfo> orbbec_devices;
    rs2::device_list rs_devices = RealSenseCamera::getAvailableDevices(ctx);
    OrbbecCamera::getAvailableDevices(&orbbec_devices);

    //# Camera Initialisation
    //#######################

    std::vector<DepthCamera*> depthCameras;

    for (auto&& dev : rs_devices)
    {
        depthCameras.push_back(new RealSenseCamera(&ctx, &dev, depthCameras.size()));
    }

    for (int i = 0; i < orbbec_devices.getSize(); i++) {
        auto dev = &orbbec_devices[i];
        try {
            depthCameras.push_back(new OrbbecCamera(dev, depthCameras.size()));
        }
        catch (const std::system_error& ex) {
            std::cout << std::endl << std::endl;
            std::cout << ex.code() << std::endl;
            std::cout << ex.code().message() << std::endl;
            std::cout << ex.what() << std::endl << std::endl;
        }
    }

    auto count = 0;
    cv::Mat frame;
    cv::Mat result;

    //# Main Loop
    //###########

    while (cv::waitKey(1) < 0 && count < NUM_FRAMES && !depthCameras.empty()) {
        count++;
        std::cout << "\r" << count << " / " << NUM_FRAMES << " Frames (" << 100 * count / NUM_FRAMES << "%)";
        for (int id = 0; id < depthCameras.size(); id++) {
            DepthCamera* cam = depthCameras[id];
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

                delete cam;
                depthCameras.erase(depthCameras.begin() + id);
            }
        }

        std::cout << std::flush;
    }

    //Shutdown
    std::cout << std::endl;
    shut_down(&depthCameras);
    
    return 0;
}

void shut_down(const std::vector<DepthCamera*>* depthCameras) {
    for (DepthCamera* cam : *depthCameras) {
        delete cam;
    }

    openni::OpenNI::shutdown();
}