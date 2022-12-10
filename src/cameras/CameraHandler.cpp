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

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

CameraHandler::CameraHandler(Camera *cam, Renderer *renderer, Logger::Logger* logger) : mp_Camera(cam), mp_Renderer(renderer), mp_Logger(logger)
{
    if (openni::OpenNI::initialize() != openni::STATUS_OK) {
        auto msg = (std::string)"Initialization of OpenNi failed: " + openni::OpenNI::getExtendedError();
        mp_Logger->log(Logger::LogLevel::ERR, msg);
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());
    }        

    for (const auto &entry : std::filesystem::directory_iterator(m_RecordingDirectory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream configJson(entry.path());
            Json::Value root;

            Json::CharReaderBuilder builder;

            builder["collectComments"] = true;

            JSONCPP_STRING errs;

            if (!parseFromStream(builder, configJson, &root, &errs)) {

                std::cout << errs << std::endl;
            }
            else {
                m_Recordings.push_back(root);
            }
        }
    }
    UpdateSessionName();
}

CameraHandler::~CameraHandler()
{
    for (auto cam : m_DepthCameras)
        delete cam;
    
    openni::OpenNI::shutdown();
}

void CameraHandler::findAllCameras()
{
    // TODO: Implement find all cameras
}

void CameraHandler::initAllCameras()
{
    for (auto cam : m_DepthCameras)
        delete cam;

    m_DepthCameras.clear();

    int id = 0;
    auto rs_cameras = RealSenseCamera::initialiseAllDevices(mp_Camera, mp_Renderer, &id, mp_Logger);
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices(mp_Camera, mp_Renderer, &id, mp_Logger);

    mp_Logger->log(Logger::LogLevel::INFO, "Queried all devices, " + std::to_string(rs_cameras.size() + orbbec_cameras.size()) + " Cameras found");
    std::cout << "[INFO] Queried all devices" << std::endl;

    m_DepthCameras.insert(m_DepthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    m_DepthCameras.insert(m_DepthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());
}

void CameraHandler::showCameras()
{
    // TODO: Implement
    //for (auto cam : m_DepthCameras)
    //    ImGui::Checkbox(cam->getCameraName().c_str(), &cam->m_isEnabled);
}

void CameraHandler::OnRender()
{
    if (m_State == Recording) {
        m_RecordedSeconds = std::chrono::system_clock::now() - m_RecordingStart;
        if (m_RecordedFrames++ > m_FrameLimit && m_LimitFrames) {
            stopRecording();
        }
    }
    
    // This sould probably be asynchronous/Multi-threaded/Parallel
    for (auto cam : m_DepthCameras)
    {
        if (cam->m_isEnabled)
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
    //# General Camera Window
    //#######################
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
        if (m_Recordings.empty()) {
            ImGui::Text("No Recordings Found!");
        }
        
        ImGui::BeginDisabled(m_State == Playback);

        for (auto recording : m_Recordings) {
            if (ImGui::TreeNode(recording["Name"].asCString())) {
                // Display information about recording, length cameras filesize

                ImGui::Text("Cameras:");

                for (auto camera : recording["Cameras"]) {
                    ImGui::Text(camera["Name"].asCString());
                    ImGui::Text(camera["Type"].asCString());
                    ImGui::Text(camera["FileName"].asCString());
                }

                if(ImGui::Button("Start Playback")){
                    m_State = Playback;
                    // Start Playback
                }

                ImGui::TreePop();
            }
        }

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
            ImGui::SameLine(); HelpMarker("Show the Live Pointcloud while recording, might decrease performance.");

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
                UpdateSessionName();
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
        ImGui::Text("Elapsed Seconds: %f.2 s", m_RecordedSeconds.count());
        ImGui::Text("Elapsed Frames: %d", m_RecordedFrames);
        ImGui::Text("Fps: %f.2", (float)m_RecordedFrames / m_RecordedSeconds.count());
        ImGui::TreePop();
    }
}

void CameraHandler::startRecording() {
    mp_Logger->log(Logger::LogLevel::INFO, "Starting recording");
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
    mp_Logger->log(Logger::LogLevel::INFO, "Stopping recording");

    m_RecordingEnd = std::chrono::system_clock::now();

    std::time_t end_time = std::chrono::system_clock::to_time_t(m_RecordingEnd);
    std::cout << "[INFO] Finished recording at " << std::ctime(&end_time)
        << "elapsed time: " << m_RecordedSeconds.count() << "s\n";

    auto configPath = m_RecordingDirectory / getFileSafeSessionName();
    configPath += ".json";

    std::fstream configJson(configPath, std::ios::out | std::ios::app);
    Json::Value root;

    root["Name"] = m_SessionName;
    root["DurationInSec"] = m_RecordedSeconds.count();
    root["Frames"] = m_RecordedFrames;

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


    m_State = Streaming;
    for (auto cam : m_DepthCameras) {
        if (cam->m_selectedForRecording) {
            cam->stopRecording();
        }
    }
}