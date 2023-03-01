#include "NuiPlaybackCamera.h"

#include <iostream>
#include <stdexcept>

#include <imgui.h>

#include "obj/PointCloud.h"
#include "obj/SkeletonDetectorNuitrack.h"
#include "utilities/Consts.h"
#include "utilities/helper/ImGuiHelper.h"

/// 
/// Constructors & Destructors
/// 

NuiPlaybackCamera::NuiPlaybackCamera(Camera* cam, Renderer* renderer, Logger::Logger* logger, std::filesystem::path recording, int* currentPlaybackFrame, Json::Value camera) :
    mp_Logger(logger), mp_CurrentPlaybackFrame(currentPlaybackFrame), m_RecordingPath(recording)
{
    mp_Logger->log("Opening NuiRecording in '" + m_RecordingPath.string() + "'");
    m_QueriedFrame = -1;
    
    queryFrame();
    auto size = m_CurrentDepthFrame.size;
    m_DepthWidth = size[1];
    m_DepthHeight = size[0];

    m_fx = camera["Fx"].asFloat();
    m_fy = camera["Fy"].asFloat();
    m_cx = camera["Cx"].asFloat();
    m_cy = camera["Cy"].asFloat();    
}

NuiPlaybackCamera::~NuiPlaybackCamera() {
    m_Frames.release();
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

void NuiPlaybackCamera::queryFrame() {
    if (m_QueriedFrame == *mp_CurrentPlaybackFrame) {
        return;
    }
    try {  
        cv::Mat frame;
        if (m_FrameBuffer.contains(*mp_CurrentPlaybackFrame)) {
            frame = m_FrameBuffer[*mp_CurrentPlaybackFrame];
        }
        else {
            cv::FileStorage frameStore((m_RecordingPath / (SkeletonDetectorNuitrack::getFrameName(*mp_CurrentPlaybackFrame) + ".yml")).string(), cv::FileStorage::READ);

            frameStore["frame"] >> frame;
            frameStore.release();
            if (frame.empty()) {
                mp_Logger->log("Frame '" + SkeletonDetectorNuitrack::getFrameName(*mp_CurrentPlaybackFrame) + "' not found!", Logger::Priority::ERR);
                return;
            }
            m_FrameBuffer.insert({ *mp_CurrentPlaybackFrame , frame});
        }
        

        std::vector<cv::Mat> channels;
        cv::split(frame, channels);

        m_CurrentDepthFrame = channels[3];
        m_CurrentDepthFrame = m_CurrentDepthFrame.reshape(0, m_DepthWidth);

        m_CurrentDepthFrame /= m_MetersPerUnit; // Transform meters to units
        m_CurrentDepthFrame.convertTo(m_CurrentDepthFrame, CV_16UC1);

        channels.pop_back();
        cv::merge(channels, m_CurrentColorFrame);
        m_CurrentColorFrame *= 255;
        m_CurrentColorFrame.convertTo(m_CurrentColorFrame, CV_8UC3);

        m_QueriedFrame = *mp_CurrentPlaybackFrame;
    }
    catch (cv::Exception& e)
    {
        mp_Logger->log(e.msg, Logger::Priority::ERR);
        return;
    }
}

const void* NuiPlaybackCamera::getDepth()
{
    queryFrame();
    return (uint16_t*)m_CurrentDepthFrame.data;
}

cv::Mat NuiPlaybackCamera::getColorFrame()
{
    queryFrame();
    return m_CurrentColorFrame;
}
