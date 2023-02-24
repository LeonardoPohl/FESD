#include "NuiPlaybackCamera.h"

#include <iostream>
#include <stdexcept>

#include <imgui.h>

#include "obj/PointCloud.h"
#include "utilities/Consts.h"
#include "utilities/helper/ImGuiHelper.h"

/// 
/// Constructors & Destructors
/// 

NuiPlaybackCamera::NuiPlaybackCamera(Camera* cam, Renderer* renderer, Logger::Logger* logger, std::filesystem::path recording, int* currentPlaybackFrame, Json::Value camera) :
    mp_Logger(logger), mp_CurrentPlaybackFrame(currentPlaybackFrame)
{

}

NuiPlaybackCamera::~NuiPlaybackCamera() {
}


/// 
/// Camera Details
/// 

std::string NuiPlaybackCamera::getType()
{
    return "NuiPlayback";
}

inline std::string NuiPlaybackCamera::getCameraName() const
{
    return this->getType();
}

void NuiPlaybackCamera::showCameraInfo() {
    if (ImGui::TreeNode(getCameraName().c_str())) {
        ImGui::Text("Nothing to see here!");
        ImGui::TreePop();
    }
}

void NuiPlaybackCamera::printDeviceInfo() const {
    printf("---\nNothing to see here!\n");
}


inline float NuiPlaybackCamera::getIntrinsics(INTRINSICS intrin) const
{
    switch (intrin)
    {
        using enum INTRINSICS;
    case FX:
        return m_fx;
    case FY:
        return m_fy;
    case CX:
        return m_cx;
    case CY:
        return m_cy;
    }
}

inline glm::mat3 NuiPlaybackCamera::getIntrinsics() const
{
    return { getIntrinsics(INTRINSICS::FX),							 0.0f, getIntrinsics(INTRINSICS::CX),
                                      0.0f,	getIntrinsics(INTRINSICS::FY), getIntrinsics(INTRINSICS::CY),
                                      0.0f,							 0.0f,							1.0f };
}

inline float NuiPlaybackCamera::getMetersPerUnit() const
{
    return m_MetersPerUnit;
}

/// 
/// Frame retreival
/// 

const void* NuiPlaybackCamera::getDepth()
{
    if (m_IsPlayback) {
        mp_PlaybackController->seek(m_DepthStream, *mp_CurrentPlaybackFrame);
    }
    else {
        int changedStreamDummy;
        openni::VideoStream* pStream = &m_DepthStream;

        // Wait a new frame
        m_RC = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
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
        mp_Logger->log(error_string, Logger::Priority::ERR);

        throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }

    if (m_IsRecording) {
        m_ColorStreamRecorder.write(getColorFrame());
    }

    return (uint16_t*)m_DepthFrameRef.getData();
}

cv::Mat NuiPlaybackCamera::getColorFrame()
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
            mp_Logger->log("No suitable OpenCV Camera has been found.", Logger::Priority::WARN);
        }
    }

    if (m_CVCameraFound) {
        if (m_IsPlayback) {
            m_ColorStream.set(cv::CAP_PROP_POS_FRAMES, *mp_CurrentPlaybackFrame);
        }
        m_ColorStream.retrieve(m_ColorFrame);
    }

    return m_ColorFrame;
}


/// 
/// Camera Settings
/// 

void NuiPlaybackCamera::CameraSettings()
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

    ImGui::EndDisabled();
    ImGui::End();
}

/// 
/// Recording
/// 

std::string NuiPlaybackCamera::startRecording(std::string sessionName)
{
    auto cameraName = getCameraName();
    std::ranges::replace(cameraName, ' ', '_');
    std::filesystem::path filepath = m_RecordingDirectory / sessionName / (cameraName + (std::string)".oni");

    m_CameraInfromation["Name"] = getCameraName();
    m_CameraInfromation["Type"] = getType();

    m_CameraInfromation["Fx"] = getIntrinsics(INTRINSICS::FX);
    m_CameraInfromation["Fy"] = getIntrinsics(INTRINSICS::FY);
    m_CameraInfromation["Cx"] = getIntrinsics(INTRINSICS::CX);
    m_CameraInfromation["Cy"] = getIntrinsics(INTRINSICS::CY);
    m_CameraInfromation["MeterPerUnit"] = getMetersPerUnit();

    m_CameraInfromation["FileName"] = sessionName + "/" + filepath.filename().string();

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
        mp_Logger->log("Orbbec Recorder is invalid", Logger::Priority::ERR);
    }

    m_ColorStreamRecorder = cv::VideoWriter{ filepath.replace_extension("avi").string(), cv::VideoWriter::fourcc('M','J','P','G'), 15, cv::Size(m_DepthWidth, m_DepthHeight) };

    m_IsEnabled = true;
    m_IsRecording = true;

    return filepath.filename().string();
}

void NuiPlaybackCamera::saveFrame() {
    std::thread depth_save_thread(&NuiPlaybackCamera::saveDepth, this);
    std::thread color_save_thread(&NuiPlaybackCamera::saveColor, this);
    depth_save_thread.join();
    color_save_thread.join();
}

void NuiPlaybackCamera::saveDepth() {
    int changedStreamDummy;
    openni::VideoStream* pStream = &m_DepthStream;

    // Wait for a new frame
    m_RC = openni::OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, READ_WAIT_TIMEOUT);
    errorHandling("Wait failed for Depth! (timeout is " + std::to_string(READ_WAIT_TIMEOUT) + " ms)");

    // Get depth frame
    m_RC = m_DepthStream.readFrame(&m_DepthFrameRef);
    errorHandling("Depth Stream read failed!");
}

void NuiPlaybackCamera::saveColor() {
    m_ColorStreamRecorder.write(getColorFrame());
}

void NuiPlaybackCamera::stopRecording()
{
    m_IsRecording = false;
    m_Recorder.stop();
    m_Recorder.destroy();
    m_ColorStreamRecorder.release();
}


/// 
/// Utils
/// 

void NuiPlaybackCamera::errorHandling(std::string error_string) {
    if (m_RC != openni::STATUS_OK)
    {
        error_string += " - ";
        error_string += openni::OpenNI::getExtendedError();
        mp_Logger->log(error_string, Logger::Priority::ERR);

        //throw std::system_error(ECONNABORTED, std::generic_category(), error_string);
    }
}