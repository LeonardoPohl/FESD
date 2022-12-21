#include "OrbbecCamera.h"

#include <iostream>
#include <vector>
#include <chrono>

#include <imgui.h>
#include <filesystem>
#include <utilities/Consts.h>

#include "obj/PointCloud.h"

constexpr int READ_WAIT_TIMEOUT = 1000;

void OrbbecCamera::getAvailableDevices(openni::Array<openni::DeviceInfo> *available_devices) {
    openni::OpenNI::enumerateDevices(available_devices);
}

std::vector<OrbbecCamera*> OrbbecCamera::initialiseAllDevices(Camera* cam, Renderer *renderer, int *starting_id, Logger::Logger* logger) {
    openni::Array<openni::DeviceInfo> orbbec_devices;
    OrbbecCamera::getAvailableDevices(&orbbec_devices);

    std::vector<OrbbecCamera*> depthCameras;

    for (int i = 0; i < orbbec_devices.getSize(); i++) {
        try {
            OrbbecCamera *d_cam = new OrbbecCamera(orbbec_devices[i], (*starting_id)++, logger);
            d_cam->makePointCloud(cam, renderer);
            depthCameras.push_back(d_cam);
            logger->log("Initialised " + depthCameras.back()->getCameraName());
        }
        catch (const std::system_error& ex) {
            logger->log(ex.code().value() + " - " + ex.code().message() + " - " + ex.what(), Logger::LogLevel::ERR);
        }
    }

    return depthCameras;
}

OrbbecCamera::OrbbecCamera(openni::DeviceInfo device_info, int camera_id, Logger::Logger* logger) :
    m_DeviceInfo(device_info), mp_Logger(logger) {
    m_CameraId = camera_id;
    printDeviceInfo();

    // Open initialised devices
    this->m_RC = this->m_Device.open(m_DeviceInfo.getUri());
    errorHandling("Couldn't open device");

    // Create depth stream
    if (this->m_Device.getSensorInfo(openni::SENSOR_DEPTH) != nullptr) {
        this->m_RC = this->m_DepthStream.create(this->m_Device, openni::SENSOR_DEPTH);
        errorHandling("Couldn't create depth stream");
    }
    else {
        mp_Logger->log("Error getting Sensor Info for the Depth Sensor", Logger::LogLevel::ERR);
        throw std::system_error(ECONNABORTED, std::generic_category(), "Error getting Sensor Info");
    }

    // Start depth stream
    this->m_RC = this->m_DepthStream.start();
    errorHandling("Couldn't start depth stream");
    if (this->m_RC == openni::STATUS_OK) {
        mp_Logger->log("Depth stream started successfully for " + getCameraName());
    }

    int changedStreamDummy;
    openni::VideoStream *pStream = &this->m_DepthStream;

    // Wait for a new (first) frame
    m_RC = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    errorHandling("Wait failed for Depth! (timeout is " + std::to_string(READ_WAIT_TIMEOUT) + " ms)");

    // Get depth frame
    m_RC = this->m_DepthStream.readFrame(&this->m_DepthFrameRef);
    errorHandling("Depth Stream read failed!");

    this->m_VideoMode = this->m_DepthFrameRef.getVideoMode();
    this->m_VideoMode.setPixelFormat(openni::PixelFormat::PIXEL_FORMAT_DEPTH_1_MM);

    this->m_DepthWidth = this->m_DepthFrameRef.getWidth();
    this->m_DepthHeight = this->m_DepthFrameRef.getHeight();
}

OrbbecCamera::~OrbbecCamera() {
    mp_Logger->log("Shutting down [Orbbec] " + this->getCameraName());

    this->m_DepthStream.stop();
    this->m_DepthStream.destroy();

    this->m_Device.close();
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
    openni::VideoStream *pStream = &this->m_DepthStream;

    // Wait a new frame
    auto m_RC = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    errorHandling("Wait failed! (timeout is " + std::to_string(READ_WAIT_TIMEOUT) + " ms)");

    // Get depth frame
    m_RC = this->m_DepthStream.readFrame(&this->m_DepthFrameRef);
    errorHandling("Depth Stream read failed!");

    // Check if the frame format is depth frame format
    if (this->m_VideoMode.getPixelFormat() != openni::PIXEL_FORMAT_DEPTH_1_MM && this->m_VideoMode.getPixelFormat() != openni::PIXEL_FORMAT_DEPTH_100_UM)
    {
        std::string error_string = "Unexpected frame format!";
        mp_Logger->log(error_string, Logger::LogLevel::ERR);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
    
    return (uint16_t*)this->m_DepthFrameRef.getData();
}

void OrbbecCamera::showCameraInfo() {
    if (ImGui::TreeNode(getCameraName().c_str())) {
        ImGui::Text("Device: %s\n", this->m_DeviceInfo.getName());
        ImGui::Text("URI: %s\n", this->m_DeviceInfo.getUri());
        ImGui::Text("USB Product Id: %d\n", this->m_DeviceInfo.getUsbProductId());
        ImGui::Text("Vendor: %s\n", this->m_DeviceInfo.getVendor());
        ImGui::TreePop();
    }
}

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

    m_isEnabled = true;

    return filepath.filename().string();
}

void OrbbecCamera::saveFrame() {
    int changedStreamDummy;
    openni::VideoStream* pStream = &this->m_DepthStream;

    // Wait for a new frame
    m_RC = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    errorHandling("Wait failed for Depth! (timeout is " + std::to_string(READ_WAIT_TIMEOUT) + " ms)");

    // Get depth frame
    m_RC = this->m_DepthStream.readFrame(&this->m_DepthFrameRef);
    errorHandling("Depth Stream read failed!");
}

void OrbbecCamera::stopRecording()
{
    m_Recorder.stop();
    m_Recorder.destroy();
}

void OrbbecCamera::OnUpdate()
{
   m_pointcloud->OnUpdate();
}

void OrbbecCamera::OnRender()
{
    m_pointcloud->OnRender();
}

void OrbbecCamera::OnImGuiRender()
{
    ImGui::Begin(getCameraName().c_str());
    ImGui::BeginDisabled(!m_isEnabled);
    m_pointcloud->OnImGuiRender();
    ImGui::EndDisabled();
    ImGui::End();
}

void OrbbecCamera::printDeviceInfo() const  {
    printf("---\nDevice: %s\n", this->m_DeviceInfo.getName());
    printf("URI: %s\n", this->m_DeviceInfo.getUri());
    printf("USB Product Id: %d\n", this->m_DeviceInfo.getUsbProductId());
    printf("Vendor: %s\n", this->m_DeviceInfo.getVendor());
}

void OrbbecCamera::errorHandling(std::string error_string) {
    if (m_RC != openni::STATUS_OK)
    {
        error_string += " - ";
        error_string += openni::OpenNI::getExtendedError();
        mp_Logger->log(error_string, Logger::LogLevel::ERR);

        //throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
}