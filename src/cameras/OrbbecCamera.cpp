#include "OrbbecCamera.h"

#include <iostream>
#include <vector>
#include <chrono>

#include <imgui.h>
#include <filesystem>

#include "obj/PointCloud.h"

constexpr int READ_WAIT_TIMEOUT = 1000;
/*
// --------------------------------
// Types
// --------------------------------
using CapturingState = enum
{
    NOT_CAPTURING,
    SHOULD_CAPTURE,
    CAPTURING,
};

using CaptureSourceType = enum
{
    CAPTURE_DEPTH_STREAM,
    CAPTURE_COLOR_STREAM,
    CAPTURE_IR_STREAM,
    CAPTURE_STREAM_COUNT
};

using StreamCaptureType = enum
{
    STREAM_CAPTURE_LOSSLESS = FALSE,
    STREAM_CAPTURE_LOSSY = TRUE,
    STREAM_DONT_CAPTURE,
};

using StreamCapturingData = struct StreamCapturingData
{
    StreamCaptureType captureType;
    const char *name;
    bool bRecording;
    openni::VideoFrameRef &(*getFrameFunc)();
    openni::VideoStream &(*getStream)();
    bool (*isStreamOn)();
    int startFrame;
};

using CapturingData = struct CapturingData
{
    openni::Recorder recorder;
    char csFileName[256];
    long long nStartOn; // time to start, in seconds
    CapturingState State;
    int nCapturedFrameUniqueID;
    char csDisplayMessage[500];
};


// --------------------------------
// Static Global Variables
// --------------------------------
CapturingData g_Capture;*/

void OrbbecCamera::getAvailableDevices(openni::Array<openni::DeviceInfo> *available_devices) {
    openni::OpenNI::enumerateDevices(available_devices);
}

std::vector<OrbbecCamera*> OrbbecCamera::initialiseAllDevices(Camera* cam, Renderer *renderer, int *starting_id, Logger::Logger* logger) {
    openni::Array<openni::DeviceInfo> orbbec_devices;
    OrbbecCamera::getAvailableDevices(&orbbec_devices);

    std::vector<OrbbecCamera*> depthCameras;

    for (int i = 0; i < orbbec_devices.getSize(); i++) {
        try {
            OrbbecCamera *d_cam = new OrbbecCamera(&orbbec_devices[i], (*starting_id)++, logger);
            d_cam->makePointCloud(cam, renderer);
            depthCameras.push_back(d_cam);
            logger->log(Logger::LogLevel::INFO, "Initialised " + depthCameras.back()->getCameraName());
        }
        catch (const std::system_error& ex) {
            logger->log(Logger::LogLevel::ERR, ex.code().value() + " - " + ex.code().message() + " - " + ex.what());
        }
    }

    return depthCameras;
}

OrbbecCamera::OrbbecCamera(const openni::DeviceInfo *device_info, int camera_id, Logger::Logger* logger) :
    _device_info(device_info), mp_Logger(logger) {
    m_CameraId = camera_id;
    printDeviceInfo();

    //# Open initialised_devices
    //##########################
    this->rc = this->_device.open(device_info->getUri());

    if (this->rc != openni::STATUS_OK)
    {
        std::string error_string = "Couldn't open device\n";
        error_string += openni::OpenNI::getExtendedError();
        mp_Logger->log(Logger::LogLevel::ERR, error_string);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    //# Create depth and color stream
    //###############################
    if (this->_device.getSensorInfo(openni::SENSOR_DEPTH) != nullptr)
    {
        //# Create depth stream
        //###############################
        this->rc = this->_depth_stream.create(this->_device, openni::SENSOR_DEPTH);
        if (this->rc != openni::STATUS_OK)
        {
            std::string error_string = "Couldn't create depth stream\n";
            error_string += openni::OpenNI::getExtendedError();
            mp_Logger->log(Logger::LogLevel::ERR, error_string);

            throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
        }
    }
    else {
        mp_Logger->log(Logger::LogLevel::ERR, "Error getting Sensor Info for " + openni::SENSOR_DEPTH);

        throw std::system_error(ECONNABORTED, std::generic_category(), "Error getting Sensor Info");
    }

    //# Start depth and color stream
    //##############################
    this->rc = this->_depth_stream.start();
    if (this->rc != openni::STATUS_OK)
    {
        std::string error_string = "Couldn't start depth stream\n";
        error_string += openni::OpenNI::getExtendedError();

        mp_Logger->log(Logger::LogLevel::ERR, error_string);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
    else {
        mp_Logger->log(Logger::LogLevel::INFO, "Depth stream started successfully for " + getCameraName());
    }

    int changedStreamDummy;
    openni::VideoStream *pStream = &this->_depth_stream;

    //# Wait a new frame
    //##################
    auto rc = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    if (rc != openni::STATUS_OK)
    {
        std::string error_string = "[ERROR]: Wait failed! (timeout is ";
        error_string += std::to_string(READ_WAIT_TIMEOUT);
        error_string += " ms)\n";
        error_string += openni::OpenNI::getExtendedError();
        mp_Logger->log(Logger::LogLevel::ERR, error_string);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    //# Get depth frame
    //#################
    rc = this->_depth_stream.readFrame(&this->_frame_ref);
    if (rc != openni::STATUS_OK)
    {
        std::string error_string = "Read failed!\n";
        error_string += openni::OpenNI::getExtendedError();
        mp_Logger->log(Logger::LogLevel::ERR, error_string);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    this->_video_mode = this->_frame_ref.getVideoMode();
    this->_video_mode.setPixelFormat(openni::PixelFormat::PIXEL_FORMAT_DEPTH_1_MM);

    this->depth_width = this->_frame_ref.getWidth();
    this->depth_height = this->_frame_ref.getHeight();
    this->max_depth = this->_depth_stream.getMaxPixelValue();
}

/// <summary>
/// Closes all video streams an stops all devices
/// </summary>
OrbbecCamera::~OrbbecCamera() {
    mp_Logger->log(Logger::LogLevel::INFO, "Shutting down [Orbbec] " + this->getCameraName());

    this->_depth_stream.stop();
    this->_depth_stream.destroy();

    this->_device.close();
}

void OrbbecCamera::makePointCloud(Camera *cam, Renderer *renderer)
{
    // -> 1 unit = 1 mm
    // -> 1 m = 1000 units 
    // -> meters per unit = 1/1000
    m_pointcloud = std::make_unique<GLObject::PointCloud>(this, cam, renderer, 1.f/1000.f);
}

const void *OrbbecCamera::getDepth()
{
    int changedStreamDummy;
    openni::VideoStream *pStream = &this->_depth_stream;

    //# Wait a new frame
    //##################
    auto rc = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    if (rc != openni::STATUS_OK)
    {
        std::string error_string = "Wait failed! (timeout is ";
        error_string += std::to_string(READ_WAIT_TIMEOUT);
        error_string += " ms)\n";
        error_string += openni::OpenNI::getExtendedError();
        mp_Logger->log(Logger::LogLevel::ERR, error_string);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
    //# Get depth frame
    //#################
    rc = this->_depth_stream.readFrame(&this->_frame_ref);
    if (rc != openni::STATUS_OK)
    {
        std::string error_string = "Read failed!\n";
        error_string += openni::OpenNI::getExtendedError();
        mp_Logger->log(Logger::LogLevel::ERR, error_string);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    //# Check if the frame format is depth frame format
    //#################################################
    if (this->_video_mode.getPixelFormat() != openni::PIXEL_FORMAT_DEPTH_1_MM && this->_video_mode.getPixelFormat() != openni::PIXEL_FORMAT_DEPTH_100_UM)
    {
        std::string error_string = "Unexpected frame format!";
        mp_Logger->log(Logger::LogLevel::ERR, error_string);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    return (uint16_t*)this->_frame_ref.getData();
}

void OrbbecCamera::showCameraInfo() {
    if (ImGui::TreeNode(getCameraName().c_str())) {
        ImGui::Text("Device: %s\n", this->_device_info->getName());
        ImGui::Text("URI: %s\n", this->_device_info->getUri());
        ImGui::Text("USB Product Id: %d\n", this->_device_info->getUsbProductId());
        ImGui::Text("Vendor: %s\n", this->_device_info->getVendor());
        ImGui::TreePop();
    }
}

//https://github.com/OpenNI/OpenNI2/blob/master/Source/Tools/NiViewer/Capture.h
std::string OrbbecCamera::startRecording(std::string sessionName, unsigned int numFrames)
{    
    //m_CameraInfromation["Name"] = getCameraName();
    //m_CameraInfromation["Type"] = getName();
    //m_CameraInfromation["FileName"] = startRecording(getFileSafeSessionName());
    //m_CameraInfromation["NumFrames"] = 0;
    
    /*
    std::filesystem::create_directory("Recordings");

    std::string fileName = std::to_string(0) + "_" + sessionName + "_" + this->getCameraName() + ".oni";

    setNumFrames(numFrames);

    openni::Status rc = g_Capture.recorder.create(fileName.c_str());

    if (rc != openni::STATUS_OK)
    {
        std::cout << "[ERROR] Failed to create recorder!" << std::endl;
        return "";
    }

    g_Capture.nStartOn = 0;
    g_Capture.State = SHOULD_CAPTURE;

    return fileName;
    */
    return "";
}

void OrbbecCamera::stopRecording()
{
    /*
    if (g_Capture.recorder.isValid())
    {
        g_Capture.recorder.destroy();
        g_Capture.State = NOT_CAPTURING;
    }*/
}

// Should these be inline?
void OrbbecCamera::OnUpdate()
{
    /*
    if (g_Capture.State == SHOULD_CAPTURE)
    {
        auto epoch = std::chrono::system_clock::now().time_since_epoch();
        auto startTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

        // check if time has arrived
        if (startTimestamp.count() >= g_Capture.nStartOn)
        {
            // check if we need to discard first frame
            // start recording
            g_Capture.recorder.attach(_depth_stream);
            g_Capture.recorder.start();
            g_Capture.State = CAPTURING;
        }
    }
    else if (g_Capture.State == CAPTURING)
    {
        if (limit_frames && decFramesLeft())
            stopRecording();
    }else
    {*/
        m_pointcloud->OnUpdate();
    //}
}

void OrbbecCamera::OnRender()
{
    m_pointcloud->OnRender();
}

void OrbbecCamera::OnImGuiRender()
{
    ImGui::Begin(getCameraName().c_str());
    ImGui::BeginDisabled(!m_isEnabled);
    /*
    if (ImGui::CollapsingHeader("Recorder")){
        if (g_Capture.State != CAPTURING && g_Capture.State != SHOULD_CAPTURE)
        {
            ImGui::Checkbox("Limit Frames", &limit_frames);

            if (limit_frames)
            {
                ImGui::SliderInt("Number of Frames", &num_frames, 1, 100000);
            }

            ImGui::SliderInt("Delay in ms", &delay, 1, 1000);

            if (ImGui::Button("Start Recording"))
            {
                auto epoch = std::chrono::system_clock::now().time_since_epoch();
                auto startTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
                //startRecording("Test Session", startTimestamp.count() + (long long)delay, num_frames);
            }
        }
        else if (g_Capture.State == SHOULD_CAPTURE)
        {
            auto epoch = std::chrono::system_clock::now().time_since_epoch();
            auto startTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
            ImGui::Text(("Capturing in: " + std::to_string(g_Capture.nStartOn - startTimestamp.count()) + " ms").c_str());
        }
        else if (g_Capture.State == CAPTURING)
        {
            ImGui::Text("Capturing");
            if (limit_frames)
            {
                ImGui::ProgressBar(1.0f - (float)frames_left / (float)num_frames);
            }
            if (ImGui::Button("Stop Recording"))
            {
                stopRecording();
            }
        }
    }
    */
    m_pointcloud->OnImGuiRender();

    ImGui::EndDisabled();
    ImGui::End();
}

void OrbbecCamera::printDeviceInfo() const  {
    printf("---\nDevice: %s\n", this->_device_info->getName());
    printf("URI: %s\n", this->_device_info->getUri());
    printf("USB Product Id: %d\n", this->_device_info->getUsbProductId());
    printf("Vendor: %s\n", this->_device_info->getVendor());
}
