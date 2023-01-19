#include "CameraHandler.h"

#include <iostream>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <execution>

#include <json/json.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <OpenNI.h>
#include <opencv2/opencv.hpp>

#include "RealsenseCamera.h"
#include "OrbbecCamera.h"

#include "obj/PointCloud.h"

#include "utilities/Consts.h"
#include "utilities/helper/ImGuiHelper.h"

CameraHandler::CameraHandler(Camera *cam, Renderer *renderer, Logger::Logger* logger) : mp_Camera(cam), mp_Renderer(renderer), mp_Logger(logger)
{
    if (openni::OpenNI::initialize() != openni::STATUS_OK) {
        auto msg = (std::string)"Initialization of OpenNi failed: " + openni::OpenNI::getExtendedError();
        mp_Logger->log(msg, Logger::Priority::ERR);
    }

    findRecordings();
    updateSessionName();
    m_SkeletonDetector = std::make_unique<SkeletonDetector>(mp_Logger);

}

CameraHandler::~CameraHandler()
{
    stopRecording();
    clearCameras();
    
    openni::OpenNI::shutdown();
}

void CameraHandler::initAllCameras()
{
    clearCameras();

    int id = 0;
    auto rs_cameras = RealSenseCamera::initialiseAllDevices(mp_Camera, mp_Renderer, &id, mp_Logger);
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices(mp_Camera, mp_Renderer, &id, mp_Logger);

    mp_Logger->log("Queried all devices, " + std::to_string(rs_cameras.size() + orbbec_cameras.size()) + " Cameras found");

    m_DepthCameras.insert(m_DepthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    m_DepthCameras.insert(m_DepthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());

    m_CamerasExist = !m_DepthCameras.empty();
    if (m_CamerasExist)
        mp_PointCloud = std::make_unique<GLObject::PointCloud>(m_DepthCameras, mp_Camera, mp_Renderer);
}

void CameraHandler::OnUpdate()
{
    if (!m_CamerasExist)
        return;

    if (m_State == Recording) {
        m_RecordedSeconds = std::chrono::system_clock::now() - m_RecordingStart;

        if (m_RecordedFrames > m_FrameLimit && m_LimitFrames) {
            stopRecording();
            return;
        }

        if (m_RecordedSeconds.count() > m_TimeLimitInS && m_LimitTime) {
            stopRecording();
            return;
        }

        m_RecordedFrames += 1;

        if (m_StreamWhileRecording) {
            mp_PointCloud->OnUpdate();
            mp_PointCloud->OnRender();
        }
        else {
            std::for_each(
                std::execution::par,
                m_DepthCameras.begin(), 
                m_DepthCameras.end(), 
                [](auto&& cam) {
                cam->saveFrame();
            });
        }
    }
    else {
        if (m_State == Streaming ||
            (m_State == Playback && !m_PlaybackPaused)) {
            mp_PointCloud->OnUpdate();
            mp_PointCloud->OnRender();
        }

        for (auto cam : m_DepthCameras)
        {
            if (cam->m_IsEnabled || (m_State == Playback && !m_PlaybackPaused))
            {
                if ((m_ShowColorFrames || m_DoSkeletonDetection) && m_State != Recording) {
                    auto frame = cam->getColorFrame();
                    if (!frame.empty()) {
                        if (m_DoSkeletonDetection) {
                            m_SkeletonDetector->drawSkeleton(frame, m_ScoreThreshold, m_ShowUncertainty);
                        }
                        cv::imshow(cam->getCameraName(), frame);
                    }
                }
            }
        }

        if (m_State == Playback) {
            m_CurrentPlaybackFrame = (m_CurrentPlaybackFrame + 1) % m_TotalPlaybackFrames;
        }
    }    
}

void CameraHandler::OnImGuiRender()
{
    ImGui::Begin("Camera Handler");

    if (ImGui::Button("Init Cameras") && m_State != Playback) {
        initAllCameras();
    }
    if (m_CamerasExist) {
        ImGui::Checkbox("Show Color Frames", &m_ShowColorFrames);

        ImGui::Checkbox("Do skeleton detection", &m_DoSkeletonDetection);
        if (m_DoSkeletonDetection) {
            ImGui::InputFloat("Score Threshold", &m_ScoreThreshold, 0.0f, 0.999f, "%.3f");
            ImGui::Checkbox("Show Uncertainty", &m_ShowUncertainty);
            ImGuiHelper::HelpMarker("Joints which are lower than the Score Thresholt will be shown in grey and the ones above will be shown ");
        }

        ImGui::Text("%d Cameras Initialised", m_DepthCameras.size());
        for (auto cam : m_DepthCameras) {
            if (m_State != Playback) {
                ImGui::Checkbox(cam->getCameraName().c_str(), &cam->m_IsEnabled);
            }
            cam->showCameraInfo();
        }
    }

    ImGui::End();
    
    if (m_CamerasExist && m_State != Playback) {
        ImGui::Begin("Recorder");

        for (auto cam : m_DepthCameras) {
            ImGui::Checkbox(("Record " + cam->getCameraName()).c_str(), &cam->m_IsSelectedForRecording);
        }

        showSessionSettings();
        showRecordingStats();

        if (m_State != Recording && ImGui::Button("Start Recording")) {
            startRecording();
        }
        else if (m_State == Recording && ImGui::Button("Stop Recording")) {
            stopRecording();
        }
        ImGui::End();
    }
    
    ImGui::Begin("Recorded Sessions");
    
    if (m_State == Playback) {
        ImGui::Checkbox("Pause Playback", &m_PlaybackPaused);
    }

    if (m_State == Playback && ImGui::Button("Stop Playback")) {
        mp_Logger->log("Stopping Playback");
        m_State = Streaming;
        clearCameras();
    }

    ImGui::BeginDisabled(m_State == Playback);

    if (ImGui::Button("Refresh Recordings")) {
        findRecordings();
    }

    if (m_Recordings.empty()) {
        ImGui::Text("No Recordings Found!");
    }

    if (ImGui::Button("(Re)Calculate Skeleton for all recordings")) {
        mp_Logger->log("Starting skeleton detection for " + std::to_string(m_Recordings.size()) + " Recordings");
        int i = 0;
        for (auto rec : m_Recordings) {
            std::cout << i++ << "/" << m_Recordings.size() << "\r";
            calculateSkeletons(rec);
        }

        mp_Logger->log("Skeleton detection done for all recordings");
    }

    showRecordings();
    ImGui::EndDisabled();

    ImGui::End();
    if (m_CamerasExist) {
        for (auto cam : m_DepthCameras) {
            cam->CameraSettings();
        }

        ImGui::Begin("PointCloud");
        mp_PointCloud->OnImGuiRender();
        ImGui::End();
    }
}

void CameraHandler::showSessionSettings() {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Session Settings")) {
        ImGui::BeginDisabled(m_State == Recording);
        if (ImGui::TreeNode("Settings")) {
            ImGui::Checkbox("Stream While Recording", &m_StreamWhileRecording);
            ImGuiHelper::HelpMarker("Show the Live Pointcloud while recording, this might decrease performance.");

            ImGui::Checkbox("Limit Frames", &m_LimitFrames);

            ImGui::BeginDisabled(!m_LimitFrames);
            ImGui::InputInt("Number of Frames", &m_FrameLimit, 1, 100);
            if (m_FrameLimit < 0) {
                m_FrameLimit = 0;
                m_LimitFrames = false;
            }
            ImGui::EndDisabled();

            ImGui::Checkbox("Limit Time", &m_LimitTime);

            ImGui::BeginDisabled(!m_LimitTime);
            ImGui::InputInt("Number of Seconds", &m_TimeLimitInS, 1, 100);
            if (m_TimeLimitInS < 0) {
                m_TimeLimitInS = 0;
                m_LimitTime = false;
            }
            ImGui::EndDisabled();
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Name")) {
            if (ImGui::Button("Update Session Name")) {
                updateSessionName();
            }
            ImGui::InputText("Session Name", &m_SessionName, 60);
            ImGui::TreePop();
        }

        ImGui::EndDisabled();
        ImGui::TreePop();
    }
}

void CameraHandler::showRecordingStats() {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Session Stats")) {
        ImGui::BeginDisabled(m_State == Recording);
        ImGui::Text("Elapsed Seconds: %.2f s", m_RecordedSeconds.count());
        ImGui::Text("Elapsed Frames: %d", m_RecordedFrames);

        if (m_LimitFrames && m_FrameLimit > 0) {
            ImGui::Text("Frame Limit:");
            ImGui::ProgressBar((float)m_RecordedFrames / (float)m_FrameLimit);
        }
        
        if (m_LimitTime && m_TimeLimitInS > 0) {
            ImGui::Text("Time Limit:");
            ImGui::ProgressBar(m_RecordedSeconds.count() / (float)m_TimeLimitInS);
        }
        ImGui::EndDisabled();

        ImGui::TreePop();
    }
}

void CameraHandler::showRecordings() {
    for (auto recording : m_Recordings) {
        bool isValid = false;
        std::string tabName = "(";
        for (auto camera : recording["Cameras"]) {
            if (camera["Type"].asString() == RealSenseCamera::getType()) {
                tabName += "RS";
                isValid = true;
            }
            else if (camera["Type"].asString() == OrbbecCamera::getType()) {
                tabName += "OB";
                isValid = true;
            }
            else {
                isValid = false;
                tabName += "??";
            }
        }

        tabName += ") - ";
        tabName += recording["Name"].asString();

        ImGui::BeginDisabled(!isValid);
        if (ImGui::TreeNode(tabName.c_str())) {
            ImGui::Text("Duration (s): %.2f", recording["DurationInSec"].asFloat());
            ImGui::Text("Recorded Frames: %d", recording["RecordedFrames"].asInt());

            ImGui::Text("Cameras:");
            for (auto camera : recording["Cameras"]) {
                if (ImGui::TreeNode(camera["Name"].asCString())) {
                    ImGui::Text("Type: %s", camera["Type"].asCString());
                    ImGui::Text("FileName: %s", camera["FileName"].asCString());
                    ImGui::TreePop();
                }
            }

            if (ImGui::Button("Start Playback")) {
                mp_Logger->log("Started Playback of Recording \"" + recording["Name"].asString() + "\"");
                m_State = Playback;

                clearCameras();

                for (auto camera : recording["Cameras"]) {
                    if (camera["Type"].asString() == RealSenseCamera::getType()) {
                        m_DepthCameras.push_back(new RealSenseCamera(mp_Camera, mp_Renderer, mp_Logger, m_RecordingDirectory / camera["FileName"].asCString(), &m_CurrentPlaybackFrame));
                    }
                    else if (camera["Type"].asString() == OrbbecCamera::getType()) {
                        m_DepthCameras.push_back(new OrbbecCamera(mp_Camera, mp_Renderer, mp_Logger, m_RecordingDirectory / camera["FileName"].asCString(), &m_CurrentPlaybackFrame));
                    }
                    else {
                        mp_Logger->log("Camera Type '" + camera["Type"].asString() + "' unknown", Logger::Priority::WARN);
                    }
                }
                m_TotalPlaybackFrames = recording["RecordedFrames"].asInt();
                m_CamerasExist = !m_DepthCameras.empty();
                if(m_CamerasExist)
                    mp_PointCloud = std::make_unique<GLObject::PointCloud>(m_DepthCameras, mp_Camera, mp_Renderer);
            }

            ImGui::TreePop();
        }
        ImGui::EndDisabled();
    }
}

void CameraHandler::startRecording() {
    mp_Logger->log("Starting recording");
    m_State = Recording;

    for (auto cam : m_DepthCameras) {
        cam->m_IsEnabled = true;
        cam->startRecording(getFileSafeSessionName());
    }

    m_RecordedFrames = 0;
    m_RecordedSeconds = std::chrono::duration<double>::zero();
    m_RecordingStart = std::chrono::system_clock::now();
}

#pragma warning(disable : 4996)
void CameraHandler::stopRecording() {
    if (m_State != Recording)
        return;

    mp_Logger->log("Stopping recording");

    m_RecordingEnd = std::chrono::system_clock::now();

    std::time_t end_time = std::chrono::system_clock::to_time_t(m_RecordingEnd);
    mp_Logger->log("Finished recording at " + (std::string)std::ctime(&end_time) + "elapsed time: " + std::to_string(m_RecordedSeconds.count()) + "s");

    auto configPath = m_RecordingDirectory / getFileSafeSessionName();
    configPath += ".json";

    std::fstream configJson(configPath, std::ios::out | std::ios::app);
    Json::Value root;

    root["Name"] = m_SessionName;
    root["DurationInSec"] = m_RecordedSeconds.count();
    root["RecordedFrames"] = m_RecordedFrames;

    Json::Value cameras;

    for (int cam_id = 0; cam_id < m_DepthCameras.size(); cam_id++) {
        auto cam = m_DepthCameras[cam_id];
        if (cam->m_IsSelectedForRecording) {
            auto cam_json = cam->getCameraConfig();

            auto r = mp_PointCloud->getRotation(cam_id);
            auto t = mp_PointCloud->getTranslation(cam_id);
            float rotation[3] = { r.x, r.y, r.z };
            float translation[3] = { t.x, t.y, t.z };

            cam_json["Rotation"] = rotation;
            cam_json["Translation"] = translation;
            
            cameras.append(cam_json);
            cam->stopRecording();
        }
    }

    root["Cameras"] = cameras;

    Json::StreamWriterBuilder builder;
    if (m_CamerasExist) {
        configJson << Json::writeString(builder, root);
        configJson.close();
    }

    updateSessionName();
    findRecordings();

    m_State = Streaming;
}

void CameraHandler::findRecordings() {
    m_Recordings.clear();

    for (const auto& entry : std::filesystem::directory_iterator(m_RecordingDirectory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream configJson(entry.path());
            Json::Value root;

            Json::CharReaderBuilder builder;

            builder["collectComments"] = true;

            JSONCPP_STRING errs;

            if (!parseFromStream(builder, configJson, &root, &errs)) {
                mp_Logger->log(errs, Logger::Priority::ERR);
            }
            else {
                m_Recordings.push_back(root);
            }
        }
    }

    mp_Logger->log("Found " + std::to_string(m_Recordings.size()) + " Recordings in '" + m_RecordingDirectory.generic_string() + "'");
}


void CameraHandler::clearCameras() {
    for (auto cam : m_DepthCameras)
        delete cam;

    m_DepthCameras.clear();
    mp_PointCloud.release();
    m_CamerasExist = false;
}

void CameraHandler::calculateSkeletons(Json::Value recording) {
    mp_Logger->log("Started Skeleton recording of \"" + recording["Name"].asString() + "\"");
    m_State = Playback;

    clearCameras();

    for (auto camera : recording["Cameras"]) {
        if (camera["Type"].asString() == RealSenseCamera::getType()) {
            m_DepthCameras.push_back(new RealSenseCamera(mp_Camera, mp_Renderer, mp_Logger, m_RecordingDirectory / camera["FileName"].asCString(), &m_CurrentPlaybackFrame));
        }
        else if (camera["Type"].asString() == OrbbecCamera::getType()) {
            m_DepthCameras.push_back(new OrbbecCamera(mp_Camera, mp_Renderer, mp_Logger, m_RecordingDirectory / camera["FileName"].asCString(), &m_CurrentPlaybackFrame));
        }
        else {
            mp_Logger->log("Camera Type '" + camera["Type"].asString() + "' unknown", Logger::Priority::WARN);
        }
    }

    m_CamerasExist = !m_DepthCameras.empty();

    if (!m_CamerasExist) {
        mp_Logger->log("No cameras could be initialised for recording \"" + recording["Name"].asString() + "\"", Logger::Priority::ERR);
        return;
    }

    m_TotalPlaybackFrames = recording["RecordedFrames"].asInt();

    m_SkeletonDetector->startRecording(recording["Name"].asString());

    for (m_CurrentPlaybackFrame = 0; m_CurrentPlaybackFrame < recording["RecordedFrames"].asInt(); m_CurrentPlaybackFrame++) {
        for (auto cam : m_DepthCameras) {
            auto frame_to_process = cam->getColorFrame();
            m_SkeletonDetector->saveFrame(frame_to_process, cam->getCameraName());
        }
    }

    m_SkeletonDetector->stopRecording();
}

void CameraHandler::updateSessionName() {
    auto tp = std::chrono::system_clock::now();
    static auto const CET = std::chrono::locate_zone("Etc/GMT-1");
    m_SessionName = "Session " + std::format("{:%FT%T}", std::chrono::zoned_time{ CET, floor<std::chrono::seconds>(tp) });
}

std::string CameraHandler::getFileSafeSessionName() {
    std::string sessionFileName = m_SessionName;
    std::ranges::replace(sessionFileName, ' ', '_');
    std::ranges::replace(sessionFileName, ':', '.');

    return sessionFileName;
}