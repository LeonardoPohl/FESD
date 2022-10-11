#include "OrbbecCamera.h"

#include <iostream>

constexpr int READ_WAIT_TIMEOUT = 1000;

using namespace openni;

void OrbbecCamera::getAvailableDevices(Array<DeviceInfo> *available_devices) {
    OpenNI::enumerateDevices(available_devices);
}

std::vector<OrbbecCamera*> OrbbecCamera::initialiseAllDevices(int *starting_id) {
    openni::Array<openni::DeviceInfo> orbbec_devices;
    OrbbecCamera::getAvailableDevices(&orbbec_devices);

    std::vector<OrbbecCamera*> depthCameras;

    for (int i = 0; i < orbbec_devices.getSize(); i++) {
        try {
            depthCameras.push_back(new OrbbecCamera(&orbbec_devices[i], (*starting_id)++));
            std::cout << "Initialised " << depthCameras.back()->getCameraName() << std::endl;
        }
        catch (const std::system_error& ex) {
            std::cout << std::endl << std::endl;
            std::cout << ex.code() << std::endl;
            std::cout << ex.code().message() << std::endl;
            std::cout << ex.what() << std::endl << std::endl;
        }
    }

    return depthCameras;
}

OrbbecCamera::OrbbecCamera(const DeviceInfo *device_info, int camera_id) :
    _device_info(device_info) {
    this->camera_id = camera_id;
    printDeviceInfo();

    //# Open initialised_devices
    //##########################
    this->rc = this->_device.open(device_info->getUri());

    if (this->rc != STATUS_OK)
    {
        std::string error_string = "Couldn't open device\n";
        error_string += OpenNI::getExtendedError();

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    //# Create depth and color stream
    //###############################
    if (this->_device.getSensorInfo(SENSOR_DEPTH) != nullptr)
    {
        //# Create depth stream
        //###############################
        this->rc = this->_depth_stream.create(this->_device, SENSOR_DEPTH);
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

    //# Start depth and color stream
    //##############################
    this->rc = this->_depth_stream.start();
    if (this->rc != STATUS_OK)
    {
        std::string error_string = "Couldn't start depth stream\n";
        error_string += OpenNI::getExtendedError();
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
    else {
        printf("Depth stream started successfully\n");
    }

    int changedStreamDummy;
    VideoStream *pStream = &this->_depth_stream;

    //# Wait a new frame
    //##################
    auto rc = OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    if (rc != STATUS_OK)
    {
        std::string error_string = "[ERROR]: Wait failed! (timeout is ";
        error_string += std::to_string(READ_WAIT_TIMEOUT);
        error_string += " ms)\n";
        error_string += OpenNI::getExtendedError();
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    //# Get depth frame
    //#################
    rc = this->_depth_stream.readFrame(&this->_frame_ref);
    if (rc != STATUS_OK)
    {
        std::string error_string = "Read failed!\n";
        error_string += OpenNI::getExtendedError();
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    this->depth_width = this->_frame_ref.getWidth();
    this->depth_height = this->_frame_ref.getHeight();
}

/// <summary>
/// Closes all video streams an stops all devices
/// </summary>
OrbbecCamera::~OrbbecCamera() {
    printf("Shutting down [Orbbec] %s...\n", this->getCameraName().c_str());
    this->_depth_stream.stop();
    this->_depth_stream.destroy();
    this->_color_stream.stop();
    this->_color_stream.destroy();

    this->_device.close();
}

const void *OrbbecCamera::getDepth()
{
    int changedStreamDummy;
    VideoStream *pStream = &this->_depth_stream;

    //# Wait a new frame
    //##################
    auto rc = OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    if (rc != STATUS_OK)
    {
        std::string error_string = "Wait failed! (timeout is ";
        error_string += std::to_string(READ_WAIT_TIMEOUT);
        error_string += " ms)\n";
        error_string += OpenNI::getExtendedError();
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
    //# Get depth frame
    //#################
    rc = this->_depth_stream.readFrame(&this->_frame_ref);
    if (rc != STATUS_OK)
    {
        std::string error_string = "Read failed!\n";
        error_string += OpenNI::getExtendedError();
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    //# Check if the frame format is depth frame format
    //#################################################
    if (this->_frame_ref.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_1_MM && this->_frame_ref.getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_100_UM)
    {
        std::string error_string = "Unexpected frame format!";
        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
    //auto *pDepth = (DepthPixel *);
    return this->_frame_ref.getData();
}

// Utils
void OrbbecCamera::printDeviceInfo() const  {
    printf("---\nDevice: %s\n", this->_device_info->getName());
    printf("URI: %s\n", this->_device_info->getUri());
    printf("USB Product Id: %d\n", this->_device_info->getUsbProductId());
    printf("Vendor: %s\n", this->_device_info->getVendor());
}
