/// Main.cpp

#include <OpenNI.h>
#include <iostream>
#include <vector>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio.hpp>  // Video write

#include "DepthCamera.h"

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

    // Initialise Devices
    std::vector<DepthCamera*> depthCameras;
    
    for (auto&& dev : rs_devices)
    {
        depthCameras.push_back(new RealSenseCamera(&ctx, &dev));
    }

    for (int i = 0; i < orbbec_devices.getSize(); i++) {
        auto dev = &orbbec_devices[i];
        try {
            depthCameras.push_back(new OrbbecCamera(dev));
        }
        catch (const std::system_error& ex) {
            std::cout << ex.code() << '\n';
            std::cout << ex.code().message() << '\n';
            std::cout << ex.what() << '\n';
        }
    }


    while (cv::waitKey(1) < 0) {
        for (DepthCamera* cam : depthCameras) {
            cam->showFrame();
        }
    }

    //Shutdown

    openni::OpenNI::shutdown();
    
    return 0;
}
