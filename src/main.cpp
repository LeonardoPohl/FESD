/// Main.cpp

#include <OpenNI.h>
#include <iostream>
#include <vector>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio.hpp>  // Video write

#include "DepthCamera.h"

void print_device_info_openni(openni::DeviceInfo deviceInfo);

void initialise_realsense();
int initialise_astra(std::vector<openni::Device> *initialised_devices, std::vector<openni::VideoStream> *video_streams);
void terminate_astra(std::vector<openni::Device> *initialised_devices, std::vector<openni::VideoStream> *video_streams);

int main() {
    std::vector<openni::Device> initialised_devices; 
    std::vector<openni::VideoStream> video_streams;

    initialise_astra(&initialised_devices, &video_streams);

    //initialise_realsense();
    terminate_astra(&initialised_devices, &video_streams);

    return 0;
}


void initialise_realsense()
{
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    // Declare RealSense pipeline, encapsulating the actual initialised_devices and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    const auto window_name = "Display Image";
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);

    cv::VideoCapture depthStreamAstra1(cv::CAP_OPENNI2_ASTRA);

    while (cv::waitKey(1) < 0 && cv::getWindowProperty(window_name, cv::WND_PROP_AUTOSIZE) >= 0)
    {
        rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
        rs2::frame depth = data.get_depth_frame().apply_filter(color_map);

        // Query frame size (width and height)
        const int w = depth.as<rs2::video_frame>().get_width();
        const int h = depth.as<rs2::video_frame>().get_height();

        // Create OpenCV matrix of size (w,h) from the colorized depth data
        cv::Mat image(cv::Size(w, h), CV_8UC3, (void*)depth.get_data(), cv::Mat::AUTO_STEP);

        // Update the window with new data
        cv::imshow(window_name, image);
    }
}

int initialise_astra(std::vector<openni::Device> *initialised_devices, std::vector<openni::VideoStream> *video_streams)
{
    //initialize openni sdk
    openni::Status rc = openni::OpenNI::initialize();
    if (rc != openni::STATUS_OK)
    {
        printf("Initialize failed\n%s\n", openni::OpenNI::getExtendedError());
        return 1;
    }
    openni::Array<openni::DeviceInfo> available_devices;
    openni::OpenNI::enumerateDevices(&available_devices);
    //std::vector<openni::Device> initialised_devices;
    //std::vector<openni::VideoStream> videoStreams;

    for (int i = 0; i < available_devices.getSize(); i++) {
        //print_device_info_openni(available_devices[i]);
        openni::Device current_device;
        rc = current_device.open(available_devices[i].getUri());
        
        //open initialised_devices
        rc = current_device.open(openni::ANY_DEVICE);
        if (rc != openni::STATUS_OK)
        {
            printf("Couldn't open device\n%s\n", openni::OpenNI::getExtendedError());
            //return 2;
        }

        print_device_info_openni(current_device.getDeviceInfo());

        openni::VideoStream color;
        openni::SensorType foo = openni::SENSOR_DEPTH;

        //create color stream
        if (current_device.getSensorInfo(openni::SENSOR_DEPTH) != NULL)
        {
            rc = color.create(current_device, openni::SENSOR_DEPTH);
            if (rc != openni::STATUS_OK)
            {
                printf("Couldn't create depth stream\n%s\n", openni::OpenNI::getExtendedError());
                continue;
            }
        }
        else {
            printf("Error getting Sensor Info for %d\n", openni::SENSOR_DEPTH);
        }

        //start color stream
        rc = color.start();
        if (rc != openni::STATUS_OK)
        {
            std::cout << rc;
            printf("Couldn't start the depth stream\n%s\n", openni::OpenNI::getExtendedError());
            continue;
        }
        else {
            video_streams->push_back(color);
            printf("Stream %d started successfully\n", openni::SENSOR_DEPTH);

        }

        initialised_devices->push_back(current_device);
    }
    openni::OpenNI::shutdown();
}

void terminate_astra(std::vector<openni::Device> *initialised_devices, std::vector<openni::VideoStream> *video_streams) {
    for (auto it = begin(*video_streams); it != end(*video_streams); ++it) {
        it->stop();
        it->destroy();
    }

    for (auto it = begin(*initialised_devices); it != end(*initialised_devices); ++it) {
        it->close();
    }
    
    openni::OpenNI::shutdown();
}

// Utils

void print_device_info_openni(openni::DeviceInfo deviceInfo) {
    printf("---\nDevice: %s\n", deviceInfo.getName());
    printf("URI: %s\n", deviceInfo.getUri());
    printf("USB Product Id: %d\n", deviceInfo.getUsbProductId());
    printf("USB Vendor Id: %d\n", deviceInfo.getUsbVendorId());
    printf("Vendor: %s\n\n", deviceInfo.getVendor());
}