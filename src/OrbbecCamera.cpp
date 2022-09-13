#include "DepthCamera.h"
#include <opencv2/highgui.hpp>
#define READ_WAIT_TIMEOUT 1000

using namespace openni;

void OrbbecCamera::getAvailableDevices(Array<DeviceInfo> *available_devices) {
    OpenNI::enumerateDevices(available_devices);
}

OrbbecCamera::OrbbecCamera(const DeviceInfo *device_info, std::string window_name){
    this->_device_info = device_info;

    printDeviceInfo();

    //open initialised_devices
    this->rc = this->_device.open(device_info->getUri());

    this->_window_name = window_name;

    if (this->rc != STATUS_OK)
    {
        std::string error_string = "Couldn't open device\n";
        error_string += OpenNI::getExtendedError();

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }


    //create color stream
    if (this->_device.getSensorInfo(SENSOR_DEPTH) != NULL)
    {
        this->rc = _depth_stream.create(this->_device, SENSOR_DEPTH);
        if (this->rc != STATUS_OK)
        {
            std::string error_string = "Couldn't create depth stream\n";
            error_string += OpenNI::getExtendedError();
            throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
        }
    }
    else {
        printf("Error getting Sensor Info for %d\n", SENSOR_DEPTH);
        throw std::system_error(ECONNABORTED, std::generic_category(), "Error getting Sensor Info");
    }

    //start color stream
    this->rc = _depth_stream.start();
    if (this->rc != STATUS_OK)
    {
        std::string error_string = "Couldn't start depth stream\n";
        error_string += OpenNI::getExtendedError();
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
    else {
        printf("Depth stream started successfully\n");
    }
}

/// <summary>
/// Closes all video streams an stops all devices
/// </summary>
OrbbecCamera::~OrbbecCamera() {
    printf("Shutting down Orbbec %s...\n", this->_window_name.c_str());
    this->_depth_stream.stop();
    this->_depth_stream.destroy();

    this->_device.close();
}

cv::Mat OrbbecCamera::getFrame() {
    int changedStreamDummy;
    VideoStream* pStream = &this->_depth_stream;

    //wait a new frame
    this->rc = OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    if (this->rc != STATUS_OK)
    {
        std::string error_string = "Wait failed! (timeout is ";
        error_string += std::to_string(READ_WAIT_TIMEOUT);
        error_string +=" ms)\n";
        error_string += OpenNI::getExtendedError();
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    //get depth frame
    this->rc = this->_depth_stream.readFrame(&this->_frame_ref);
    if (this->rc != STATUS_OK)
    {
        std::string error_string = "Read failed!\n";
        error_string += OpenNI::getExtendedError();
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    //check if the frame format is depth frame format
    if (this->_frame_ref.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_1_MM && this->_frame_ref.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_100_UM)
    {
        std::string error_string = "Unexpected frame format!";
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
    //https://opencv.org/working-with-orbbec-astra-3d-cameras-using-opencv/ for the matrix type
    DepthPixel* pDepth = (DepthPixel*)this->_frame_ref.getData();
	return cv::Mat(cv::Size(this->_frame_ref.getWidth(), this->_frame_ref.getHeight()), CV_16UC1, pDepth, cv::Mat::AUTO_STEP);
}

// Utils
void OrbbecCamera::printDeviceInfo() {
    printf("---\nDevice: %s\n", this->_device_info->getName());
    printf("URI: %s\n", this->_device_info->getUri());
    printf("USB Product Id: %d\n", this->_device_info->getUsbProductId());
    printf("Vendor: %s\n", this->_device_info->getVendor());
}