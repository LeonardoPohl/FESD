#include "OrbbecCamera.h"

#include <iostream>
#include <vector>
#include <chrono>

#include <imgui.h>
#include <filesystem>
#include <utilities/Consts.h>

#include "obj/PointCloud.h"

constexpr int READ_WAIT_TIMEOUT = 1000;

/// 
/// Constructors & Destructors
/// 

OrbbecCamera::OrbbecCamera(openni::DeviceInfo deviceInfo, Camera* cam, Renderer* renderer, int camera_id, Logger::Logger* logger) :
    m_DeviceInfo(deviceInfo), mp_Logger(logger) {
    m_CameraId = camera_id;
    printDeviceInfo();

    // Open initialised devices
    m_RC = m_Device.open(m_DeviceInfo.getUri());
    errorHandling("Couldn't open device");

    // Create depth stream
    if (m_Device.getSensorInfo(openni::SENSOR_DEPTH) != nullptr) {
        m_RC = m_DepthStream.create(m_Device, openni::SENSOR_DEPTH);
        errorHandling("Couldn't create depth stream");
    }
    else {
        mp_Logger->log("Error getting Sensor Info for the Depth Sensor", Logger::LogLevel::ERR);
        throw std::system_error(ECONNABORTED, std::generic_category(), "Error getting Sensor Info");
    }

    // Start depth stream
    m_RC = m_DepthStream.start();
    errorHandling("Couldn't start depth stream");
    if (m_RC == openni::STATUS_OK) {
        mp_Logger->log("Depth stream started successfully for " + getCameraName());
    }

    int changedStreamDummy;
    openni::VideoStream* pStream = &m_DepthStream;

    // Wait for a new (first) frame
    m_RC = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    errorHandling("Wait failed for Depth! (timeout is " + std::to_string(READ_WAIT_TIMEOUT) + " ms)");

    // Get depth frame
    m_RC = m_DepthStream.readFrame(&m_DepthFrameRef);
    errorHandling("Depth Stream read failed!");

    m_VideoMode = m_DepthFrameRef.getVideoMode();
    m_VideoMode.setPixelFormat(openni::PixelFormat::PIXEL_FORMAT_DEPTH_1_MM);

    m_DepthWidth = m_DepthFrameRef.getWidth();
    m_DepthHeight = m_DepthFrameRef.getHeight();

    m_ColorStream = cv::VideoCapture{ 0, cv::CAP_DSHOW };

    m_ColorStream.set(cv::CAP_PROP_FRAME_WIDTH, m_DepthWidth);
    m_ColorStream.set(cv::CAP_PROP_FRAME_HEIGHT, m_DepthHeight);

    getColorFrame();

    // -> 1 unit = 1 mm
    // -> 1 m = 1000 units
    // -> meters per unit = 1/1000
    m_PointCloud = std::make_unique<GLObject::PointCloud>(this, cam, renderer, 1.f / 1000.f);
}

OrbbecCamera::OrbbecCamera(Camera* cam, Renderer* renderer, Logger::Logger* logger, std::filesystem::path recording) :
    mp_Logger(logger) {
    m_RC = m_Device.open(recording.string().c_str());
    errorHandling("Couldn't open Recording!");

    m_RC = m_DepthStream.create(m_Device, openni::SENSOR_DEPTH);
    errorHandling("Couldn't create depth stream!");

    m_RC = m_DepthStream.start();
    errorHandling("Couldn't start depth stream!");

    mp_PlaybackController = m_Device.getPlaybackControl();

    mp_PlaybackController->seek(m_DepthStream, 0);

    m_RC = m_DepthStream.readFrame(&m_DepthFrameRef);
    errorHandling("Depth Stream read failed!");

    m_VideoMode = m_DepthFrameRef.getVideoMode();
    m_VideoMode.setPixelFormat(openni::PixelFormat::PIXEL_FORMAT_DEPTH_1_MM);

    try {
        m_ColorStream = cv::VideoCapture{ recording.replace_extension("avi").string() };
    }
    catch (...) {
        m_PlaybackHasRGBStream = false;
        mp_Logger->log("RGB Recording '" + recording.replace_extension("avi").string() + "' not found for " + getCameraName(), Logger::LogLevel::WARNING);
    }
    
    m_DepthWidth = m_DepthFrameRef.getWidth();
    m_DepthHeight = m_DepthFrameRef.getHeight();

    m_IsPlayback = true;
    m_IsEnabled = true;
    m_CVCameraFound = true;

    m_PointCloud = std::make_unique<GLObject::PointCloud>(this, cam, renderer, 1.f / 1000.f);
}

OrbbecCamera::~OrbbecCamera() {
    mp_Logger->log("Shutting down [Orbbec] " + getCameraName());

    m_DepthStream.stop();
    m_DepthStream.destroy();

    m_Device.close();
}

/// 
/// Initialise all devices
/// 

void OrbbecCamera::getAvailableDevices(openni::Array<openni::DeviceInfo> *available_devices) {
    openni::OpenNI::enumerateDevices(available_devices);
}

std::vector<OrbbecCamera*> OrbbecCamera::initialiseAllDevices(Camera* cam, Renderer *renderer, int *starting_id, Logger::Logger* logger) {
    openni::Array<openni::DeviceInfo> orbbec_devices;
    OrbbecCamera::getAvailableDevices(&orbbec_devices);

    std::vector<OrbbecCamera*> depthCameras;

    for (int i = 0; i < orbbec_devices.getSize(); i++) {
        try {
            depthCameras.push_back(new OrbbecCamera(orbbec_devices[i], cam, renderer, (*starting_id)++, logger));
            logger->log("Initialised " + depthCameras.back()->getCameraName());
        }
        catch (const std::system_error& ex) {
            logger->log(ex.code().value() + " - " + ex.code().message() + " - " + ex.what(), Logger::LogLevel::ERR);
        }
    }

    return depthCameras;
}

/// 
/// Camera Details
/// 

std::string OrbbecCamera::getType() 
{
    return "Orbbec"; 
}

inline std::string OrbbecCamera::getCameraName() const 
{
    return this->getType() + " Camera " + std::to_string(this->m_CameraId);
}

void OrbbecCamera::showCameraInfo() {
    if (ImGui::TreeNode(getCameraName().c_str())) {
        ImGui::Text("Device: %s\n", m_DeviceInfo.getName());
        ImGui::Text("URI: %s\n", m_DeviceInfo.getUri());
        ImGui::Text("USB Product Id: %d\n", m_DeviceInfo.getUsbProductId());
        ImGui::Text("Vendor: %s\n", m_DeviceInfo.getVendor());
        ImGui::TreePop();
    }
}

void OrbbecCamera::printDeviceInfo() const {
    printf("---\nDevice: %s\n", m_DeviceInfo.getName());
    printf("URI: %s\n", m_DeviceInfo.getUri());
    printf("USB Product Id: %d\n", m_DeviceInfo.getUsbProductId());
    printf("Vendor: %s\n", m_DeviceInfo.getVendor());
}


inline float OrbbecCamera::getIntrinsics(INTRINSICS intrin) const
{
    //https://towardsdatascience.com/inverse-projection-transformation-c866ccedef1c
    auto fx = getDepthStreamWidth() / (2.f * tan(m_hfov / 2.f));
    auto fy = getDepthStreamHeight() / (2.f * tan(m_vfov / 2.f));
    auto cx = getDepthStreamWidth() / 2;
    auto cy = getDepthStreamHeight() / 2;

    switch (intrin)
    {
        using enum INTRINSICS;
    case FX:
        return fx;
    case FY:
        return fy;
    case CX:
        return (float)cx;
    case CY:
        return (float)cy;
    }
}

inline glm::mat3 OrbbecCamera::getIntrinsics() const
{
    return { getIntrinsics(INTRINSICS::FX),							 0.0f, getIntrinsics(INTRINSICS::CX),
                                      0.0f,	getIntrinsics(INTRINSICS::FY), getIntrinsics(INTRINSICS::CY),
                                      0.0f,							 0.0f,							1.0f };
}

/// 
/// Frame retreival
/// 

const void *OrbbecCamera::getDepth()
{
    if (m_IsPlayback) {
        mp_PlaybackController->seek(m_DepthStream, m_CurrentPlaybackFrame);
        m_CurrentPlaybackFrame = ++m_CurrentPlaybackFrame % mp_PlaybackController->getNumberOfFrames(m_DepthStream);
    }
    else {
        int changedStreamDummy;
        openni::VideoStream* pStream = &m_DepthStream;

        // Wait a new frame
        auto m_RC = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
        errorHandling("Wait failed! (timeout is " + std::to_string(READ_WAIT_TIMEOUT) + " ms)");
    }

    // Get depth frame
    m_RC = m_DepthStream.readFrame(&m_DepthFrameRef);
    errorHandling("Depth Stream read failed!");

    // Check if the frame format is depth frame format
    if (!m_IsPlayback && m_VideoMode.getPixelFormat() != openni::PIXEL_FORMAT_DEPTH_1_MM && m_VideoMode.getPixelFormat() != openni::PIXEL_FORMAT_DEPTH_100_UM)
    {
        std::cout << m_VideoMode.getPixelFormat() << std::endl;
        std::string error_string = "Unexpected frame format!";
        mp_Logger->log(error_string, Logger::LogLevel::ERR);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    if (m_IsRecording) {
        m_ColorStreamRecorder.write(getColorFrame());
    }
    
    return (uint16_t*)m_DepthFrameRef.getData();
}

cv::Mat OrbbecCamera::getColorFrame()
{
    if (!m_CVCameraFound) {
        while (!m_ColorStream.grab() && m_CVCameraId < m_CVCameraSearchDepth) {
            m_CVCameraId += 1;
            m_ColorStream = cv::VideoCapture{ m_CVCameraId, cv::CAP_DSHOW };

            m_ColorStream.set(cv::CAP_PROP_FRAME_WIDTH, m_DepthWidth);
            m_ColorStream.set(cv::CAP_PROP_FRAME_HEIGHT, m_DepthHeight);
        }

        if (!m_CVCameraFound && m_CVCameraId < m_CVCameraSearchDepth) {
            m_CVCameraFound = true;
        }
        else if (m_CVCameraId == m_CVCameraSearchDepth) {
            m_CVCameraId += 1;
            mp_Logger->log("No suitable OpenCV Camera has been found.", Logger::LogLevel::WARNING);
        }
    }

    if (m_CVCameraFound) {
        m_ColorStream.retrieve(m_ColorFrame);
    }
    

    return m_ColorFrame;
}


/// 
/// Frame update
/// 

void OrbbecCamera::OnUpdate()
{
   m_PointCloud->OnUpdate();
}

void OrbbecCamera::OnRender()
{
    m_PointCloud->OnRender();
}

void OrbbecCamera::OnImGuiRender()
{
    ImGui::Begin(getCameraName().c_str());
    ImGui::BeginDisabled(!m_IsEnabled);
    
    ImGui::BeginDisabled();
    ImGui::Checkbox("RGB Camera found", &m_CVCameraFound);
    ImGui::EndDisabled();

    if (ImGui::Button("Continue Search")) {
        m_CVCameraFound = false;
    }
    if (ImGui::Button("Restart Search")) {
        m_CVCameraFound = false;
        m_CVCameraId = 0;
    }

    m_PointCloud->OnImGuiRender();
    ImGui::EndDisabled();
    ImGui::End();
}

/// 
/// Recording
/// 

std::string OrbbecCamera::startRecording(std::string sessionName)
{
    auto cameraName = getCameraName();
    std::ranges::replace(cameraName, ' ', '_');
    std::filesystem::path filepath = m_RecordingDirectory / (sessionName + "_" + cameraName + ".oni");

    m_CameraInfromation["Name"] = getCameraName();
    m_CameraInfromation["Type"] = getType();
    m_CameraInfromation["FileName"] = filepath.filename().string();

    m_RC = m_Recorder.create(filepath.string().c_str());
    errorHandling("Recorder Creation Failed!");

    m_RC = m_Recorder.attach(m_DepthStream);
    errorHandling("Failed attaching depth steam!");

    m_RC = m_Recorder.start();
    errorHandling("Failed starting Recorder!");

    if (m_Recorder.isValid()) {
        mp_Logger->log("Created Orbbec Recorder");
    }
    else {
        mp_Logger->log("Orbbec Recorder is invalid", Logger::LogLevel::ERR);
    }

    m_ColorStreamRecorder = cv::VideoWriter{ filepath.replace_extension("avi").string(), cv::VideoWriter::fourcc('M','J','P','G'), 30, cv::Size(m_DepthWidth, m_DepthHeight)};

    m_IsEnabled = true;
    m_IsRecording = true;

    return filepath.filename().string();
}

void OrbbecCamera::saveFrame() {
    int changedStreamDummy;
    openni::VideoStream* pStream = &m_DepthStream;

    // Wait for a new frame
    m_RC = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    errorHandling("Wait failed for Depth! (timeout is " + std::to_string(READ_WAIT_TIMEOUT) + " ms)");

    // Get depth frame
    m_RC = m_DepthStream.readFrame(&m_DepthFrameRef);
    errorHandling("Depth Stream read failed!");

    m_ColorStreamRecorder.write(getColorFrame());
}

void OrbbecCamera::stopRecording()
{
    m_IsRecording = false;
    m_Recorder.stop();
    m_Recorder.destroy();
    m_ColorStreamRecorder.release();
}


/// 
/// Utils
/// 

void OrbbecCamera::errorHandling(std::string error_string) {
    if (m_RC != openni::STATUS_OK)
    {
        error_string += " - ";
        error_string += openni::OpenNI::getExtendedError();
        mp_Logger->log(error_string, Logger::LogLevel::ERR);

        //throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
}