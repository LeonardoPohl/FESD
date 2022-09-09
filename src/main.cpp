#include <OpenNI.h>
#include <iostream>
#include <vector>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat)
#include <opencv2/videoio.hpp>  // Video write

void print_device_info(openni::DeviceInfo deviceInfo);

int main() {
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    const auto window_name = "Display Image";
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);

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


    //initialize openni sdk
    openni::Status rc = openni::OpenNI::initialize();
    if (rc != openni::STATUS_OK)
    {
        printf("Initialize failed\n%s\n", openni::OpenNI::getExtendedError());
        return 1;
    }
    openni::Array<openni::DeviceInfo> devices;
    openni::OpenNI::enumerateDevices(&devices);
    openni::Device device;
    std::vector<openni::VideoStream> videoStreams;
    for (int i = 0; i < devices.getSize(); i++) {
        //print_device_info(devices[i]);
        rc = device.open(devices[i].getUri());

        //open device
        rc = device.open(openni::ANY_DEVICE);
        if (rc != openni::STATUS_OK)
        {
            printf("Couldn't open device\n%s\n", openni::OpenNI::getExtendedError());
            //return 2;
        }

        print_device_info(device.getDeviceInfo());

        openni::VideoStream color;
        for (int fooInt = openni::SENSOR_IR; fooInt != openni::SENSOR_DEPTH+1; fooInt++)
        {
            openni::SensorType foo = static_cast<openni::SensorType>(fooInt);
            device.getSensorInfo(foo);
            //create color stream
            if (device.getSensorInfo(foo) != NULL)
            {
                rc = color.create(device, foo);
                if (rc != openni::STATUS_OK)
                {
                    printf("Couldn't create depth stream\n%s\n", openni::OpenNI::getExtendedError());
                    continue;
                }
            }
            else {
                printf("Error getting Sensor Info for %d\n", foo);
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
                printf("Stream %d started successfully\n", foo);
            }
        }
    }
    openni::OpenNI::shutdown();
    return 0;
}

void print_device_info(openni::DeviceInfo deviceInfo) {
    printf("---\nDevice: %s\n", deviceInfo.getName());
    printf("URI: %s\n", deviceInfo.getUri());
    printf("USB Product Id: %d\n", deviceInfo.getUsbProductId());
    printf("USB Vendor Id: %d\n", deviceInfo.getUsbVendorId());
    printf("Vendor: %s\n\n", deviceInfo.getVendor());
}
