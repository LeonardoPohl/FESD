#include "CameraHandler.h"

#include <imgui.h>

#include "RealsenseCamera.h"
#include "OrbbecCamera.h"

#include <OpenNI.h>
#include <iostream>
#include <obj/PointCloud.h>
#include <ctime>
#include <chrono>
#include <fstream>
#include <json/json.h>

CameraHandler::CameraHandler(Camera *cam, Renderer *renderer) : mp_Camera(cam), mp_Renderer(renderer)
{
    if (openni::OpenNI::initialize() != openni::STATUS_OK)
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());

    for (const auto &entry : fs::directory_iterator(m_RecordingDirectory))
        m_Recordings.push_back(entry.path());
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
    auto rs_cameras = RealSenseCamera::initialiseAllDevices(mp_Camera, mp_Renderer, &id);
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices(mp_Camera, mp_Renderer, &id);

    std::cout << "[INFO] Queried all devices" << std::endl;

    m_DepthCameras.insert(m_DepthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    m_DepthCameras.insert(m_DepthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());
}

void CameraHandler::showCameras()
{
    // TODO: Implement
    for (auto cam : m_DepthCameras)
        ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);
}

void CameraHandler::OnRender()
{
    for (auto cam : m_DepthCameras)
    {
        if (cam->is_enabled)
        {
            cam->OnUpdate();
            cam->OnRender();
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

    for (auto cam : m_DepthCameras)
        ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);
    
    if (!m_DepthCameras.empty()) {
        ImGui::Text("Recording");

        for (auto cam : m_DepthCameras)
            ImGui::Checkbox(("Record " + cam->getCameraName()).c_str(), &cam->is_recording);

        ImGui::BeginDisabled(m_State == Recording);

        ImGui::InputText("Session Name", m_SessionName, 60);

        if (ImGui::Button("Start Recording")) {
            std::string sessionFileName = m_SessionName;
            sessionFileName += ".json";
            std::ranges::replace(sessionFileName, ' ', '_');

            std::fstream configJson(m_RecordingDirectory / sessionFileName, std::ios::out | std::ios::app);
            Json::Value root;

            root["DurationInSec"] = -1;

            Json::Value cameras;

            for (auto cam : m_DepthCameras) {
                if (cam->is_recording) {
                    cam->is_enabled = true;
                    Json::Value camera;

                    camera["Name"] = cam->getCameraName();
                    camera["Type"] = cam->getName();
                    camera["FileName"] = cam->getName();
                    cameras.append(camera);

                    //cam->startRecording(m_SessionName);
                }
            }

            root["Cameras"] = cameras;

            Json::StreamWriterBuilder builder;

            configJson << Json::writeString(builder, root);
            configJson.close();

            
            m_State = Recording;
        }

        ImGui::EndDisabled();
        
        ImGui::BeginDisabled(m_State != Recording);

        // Make it possible to stop recording
        // Count number of frames and file size maybe

        ImGui::EndDisabled();
    }
    else {
        ImGui::Text("Playback");

        if (m_Recordings.empty()) {
            ImGui::Text("No Recordings Found!");
        }
        
        ImGui::BeginDisabled(m_DoingPlayback);

        for (auto recording : m_Recordings) {
            if (ImGui::Button(recording.filename().string().c_str())) {
                m_DoingPlayback = true;
                // Start Playback
            }
        }

        ImGui::EndDisabled();
    }

    ImGui::End();

    for (auto cam : m_DepthCameras)
    {
        if (cam->is_enabled)
        {
            ImGui::Begin(cam->getCameraName().c_str());
            cam->OnImGuiRender();
            ImGui::End();
        }
    }
}