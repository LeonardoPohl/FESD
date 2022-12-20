#include "CameraHandler.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "RealsenseCamera.h"
#include "OrbbecCamera.h"

#include <OpenNI.h>
#include <iostream>
#include <obj/PointCloud.h>
#include <ctime>
#include <fstream>
#include <filesystem>

#include <utilities/Consts.h>
#include <utilities/helper/ImGuiHelper.h>

CameraHandler::CameraHandler(Camera *cam, Renderer *renderer, Logger::Logger* logger) : mp_Camera(cam), mp_Renderer(renderer), mp_Logger(logger)
{
    if (openni::OpenNI::initialize() != openni::STATUS_OK) {
        auto msg = (std::string)"Initialization of OpenNi failed: " + openni::OpenNI::getExtendedError();
        mp_Logger->log(msg, Logger::LogLevel::ERR);
    }        

    findRecordings();
    updateSessionName();
}

CameraHandler::~CameraHandler()
{
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
}

void CameraHandler::OnRender()
{
    if (m_State == Recording) {
        m_RecordedSeconds = std::chrono::system_clock::now() - m_RecordingStart;
        m_RecordedFrames += 1;

        if (m_RecordedFrames > m_FrameLimit && m_LimitFrames) {
            stopRecording();
            return;
        }

        if (m_RecordedSeconds.count() > m_TimeLimit && m_LimitTime) {
            stopRecording();
            return;
        }
    }
    
    // This sould probably be asynchronous/Multi-threaded/Parallel
    for (auto cam : m_DepthCameras)
    {
        if (cam->m_isEnabled || m_State == Playback)
        {
            if (m_State == Recording && !m_StreamWhileRecording) {
                cam->saveFrame();
            }
            else {
                cam->OnUpdate();
                cam->OnRender();
            }
        }
    }
}

void CameraHandler::OnImGuiRender()
{
    ImGui::Begin("Camera Handler");

    if (ImGui::Button("Init Cameras")) {
        initAllCameras();
    }
    if (!m_DepthCameras.empty()) {
        ImGui::Text("%d Cameras Found", m_DepthCameras.size());
        for (auto cam : m_DepthCameras) {
            ImGui::Checkbox(cam->getCameraName().c_str(), &cam->m_isEnabled);
            cam->showCameraInfo();
        }
    }
    
    if (!m_DepthCameras.empty() && ImGui::CollapsingHeader("Recording")) {
        for (auto cam : m_DepthCameras) {
            ImGui::Checkbox(("Record " + cam->getCameraName()).c_str(), &cam->m_selectedForRecording);
        }

        showSessionSettings();
        showRecordingStats();

        if (m_State != Recording && ImGui::Button("Start Recording")) {
            startRecording();
        }
        else if (m_State == Recording && ImGui::Button("Stop Recording")) {
            stopRecording();
        }
    }
    
    if (ImGui::CollapsingHeader("Recorded Sessions")) {
        if (m_State == Playback && ImGui::Button("Stop Playback")) {
            mp_Logger->log("Stopping Playback");
            m_State = Streaming;
            for (auto cam : m_DepthCameras)
                delete cam;
            m_DepthCameras.clear();
        }

        ImGui::BeginDisabled(m_State == Playback);

        if (ImGui::Button("Refresh Recordings")) {
            findRecordings();
        }

        if (m_Recordings.empty()) {
            ImGui::Text("No Recordings Found!");
        }

        showRecordings();
        ImGui::EndDisabled();
    }

    ImGui::End();

    for (auto cam : m_DepthCameras)
    {
        cam->OnImGuiRender();
        ImGui::End();
    }
}

void CameraHandler::showSessionSettings() {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Session Settings")) {
        ImGui::BeginDisabled(m_State == Recording);
        if (ImGui::TreeNode("Settings")) {
            ImGui::Checkbox("Stream While Recording", &m_StreamWhileRecording);
            ImGui::SameLine(); ImGuiHelper::HelpMarker("Show the Live Pointcloud while recording, might decrease performance.");

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
            ImGui::InputInt("Number of Seconds", &m_TimeLimit, 1, 100);
            if (m_TimeLimit < 0) {
                m_TimeLimit = 0;
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
        
        if (m_LimitTime && m_TimeLimit > 0) {
            ImGui::Text("Time Limit:");
            ImGui::ProgressBar(m_RecordedSeconds.count() / (float)m_TimeLimit);
        }
        ImGui::EndDisabled();

        ImGui::TreePop();
    }
}

void CameraHandler::showRecordings() {
    for (auto recording : m_Recordings) {
        if (ImGui::TreeNode(recording["Name"].asCString())) {
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
                        m_DepthCameras.push_back(new RealSenseCamera(mp_Camera, mp_Renderer, mp_Logger, m_RecordingDirectory / camera["FileName"].asCString()));
                    }
                }
            }

            ImGui::TreePop();
        }
    }
}

void CameraHandler::startRecording() {
    mp_Logger->log("Starting recording");
    m_State = Recording;
    m_RecordedFrames = 0;
    m_RecordedSeconds = std::chrono::duration<double>::zero();
    m_RecordingStart = std::chrono::system_clock::now();
    for (auto cam : m_DepthCameras) {
        cam->m_isEnabled = true;
        cam->startRecording(getFileSafeSessionName());
    }
}

#pragma warning(disable : 4996)
void CameraHandler::stopRecording() {
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

    for (auto cam : m_DepthCameras) {
        if (cam->m_selectedForRecording) {
            cameras.append(cam->getCameraConfig());
        }
    }
    std::cout << cameras;
    root["Cameras"] = cameras;

    Json::StreamWriterBuilder builder;

    configJson << Json::writeString(builder, root);
    configJson.close();

    updateSessionName();
    findRecordings();

    m_State = Streaming;
    for (auto cam : m_DepthCameras) {
        if (cam->m_selectedForRecording) {
            cam->stopRecording();
        }
    } 
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
                mp_Logger->log(errs, Logger::LogLevel::ERR);
            }
            else {
                m_Recordings.push_back(root);
            }
        }
    }
}