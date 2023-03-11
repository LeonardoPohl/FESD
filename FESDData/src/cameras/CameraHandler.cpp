#include "CameraHandler.h"

#include <iostream>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <execution>

#include <json/json.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <OpenNI.h>

#include "RealsenseCamera.h"
#include "OrbbecCamera.h"
#include "NuiPlaybackCamera.h"

#include "obj/PointCloud.h"
#include "obj/Error.h"

#include "utilities/Consts.h"
#include "utilities/helper/ImGuiHelper.h"
#include "utilities/Utils.h"
#include "utilities/helper/GLFWHelper.h"
#include "utilities/ConvertRecordings.h"

CameraHandler::CameraHandler(Camera *cam, Renderer *renderer, Logger::Logger* logger) : mp_Camera(cam), mp_Renderer(renderer), mp_Logger(logger)
{
    if (openni::OpenNI::initialize() != openni::STATUS_OK) {
        auto msg = (std::string)"Initialization of OpenNi failed: " + openni::OpenNI::getExtendedError();
        mp_Logger->log(msg, Logger::Priority::ERR);
    }

    findRecordings();
    updateSessionName();
    m_SkeletonDetectorOpenPose = std::make_unique<SkeletonDetectorOpenPose>(mp_Logger);

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
        mp_PointCloud = std::make_unique<GLObject::PointCloud>(m_DepthCameras, mp_Camera, mp_Logger, mp_Renderer);
}

void CameraHandler::OnUpdate()
{
    if (!m_CamerasExist && !(m_SessionParams.EstimateSkeleton && (m_State != Streaming && m_State != Playback)))
        return;

    if (m_State == Streaming) {
        stream();
    }
    else if (m_State == Recording) {
        record();
    }
    else if (m_State == Playback) {
        playback();
    }
}

void CameraHandler::OnImGuiRender()
{
    if (m_State == Recording) {
        showRecordingStats();
        m_SessionParams.showCurrentSession();

        return;
    }
    else if (m_State == Countdown) {
        countdown();
    }
    
    showGeneralGui();    
    showRecordingGui();    
    showPlaybackGui();

    if (m_CamerasExist) {
        for (auto cam : m_DepthCameras) {
            cam->CameraSettings();
        }

        ImGui::Begin("PointCloud");
        mp_PointCloud->OnImGuiRender();
        ImGui::End();
    }
}

void CameraHandler::showGeneralGui()
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
}


void CameraHandler::stream()
{
    mp_PointCloud->OnUpdate();
    mp_PointCloud->OnRender();

    for (auto cam : m_DepthCameras)
    {
        if (cam->m_IsEnabled && (m_ShowColorFrames || m_DoSkeletonDetection)) {
            auto frame = cam->getColorFrame();
            if (!frame.empty()) {
                if (m_DoSkeletonDetection) {
                    m_SkeletonDetectorOpenPose->drawSkeleton(frame, m_ScoreThreshold, m_ShowUncertainty);
                }

                ImGui::Begin((cam->getCameraName() + (std::string)" Color Frame").c_str());
                ImGuiHelper::showImage(frame);
                ImGui::End();
            }
        }
    }
}

/// 
/// Recording
/// 

void CameraHandler::showRecordingGui()
{
    if (m_State == RecordingPre) {
        if (m_SessionParams.manipulateSessionParameters()) {
            initRecording();
        }
    }

    if (m_CamerasExist && m_State != Playback) {
        ImGui::Begin("Recorder");
        ImGui::BeginDisabled(m_State == RecordingPre);
        for (auto cam : m_DepthCameras) {
            ImGui::Checkbox(("Record " + cam->getCameraName()).c_str(), &cam->m_IsSelectedForRecording);
        }

        if (m_State != RecordingPre && ImGui::Button("Start Recording Prep")) {
            m_State = RecordingPre;
        }
        ImGui::EndDisabled();

        if (m_State == RecordingPre && ImGui::Button("Stop Recording Prep")) {
            m_State = Streaming;
        }

        ImGui::End();
    }
}

void CameraHandler::initRecording() {
    mp_Logger->log("Initialise recording");

    std::filesystem::create_directory(m_RecordingDirectory / getFileSafeSessionName(m_SessionName));

    if (m_SessionParams.EstimateSkeleton) {
        if (!m_DepthCameras.empty()) {
            auto intrin = m_DepthCameras[0]->getIntrinsics();
            m_SkeletonDetectorNuitrack = std::make_unique<SkeletonDetectorNuitrack>(mp_Logger, intrin);
        }

        if (m_SkeletonDetectorNuitrack) {
            clearCameras();
            m_SkeletonDetectorNuitrack->startRecording(getFileSafeSessionName(m_SessionName));
        }
    }
    else {
        for (auto cam : m_DepthCameras) {
            cam->m_IsEnabled = true;
            cam->startRecording(getFileSafeSessionName(m_SessionName));
        }
    }

    // Sound an alarm that the countdown is starting
    std::cout << "\a";

    countdown();
}

void CameraHandler::countdown() {
    m_State = Countdown;
    
    if (m_SessionParams.countDown()) {
        startRecording();
    }
    else if (m_SessionParams.cancelRecording()) {
        updateSessionName();
        findRecordings();

        if (!m_SessionParams.EstimateSkeleton) {
            clearCameras();

            initAllCameras();
        }

        m_State = Streaming;
    }
}

void CameraHandler::startRecording() {
    mp_Logger->log("Starting recording");
    m_State = Recording;

    m_RecordedFrames = 0;
    m_RecordedSeconds = std::chrono::duration<double>::zero();
    m_RecordingStart = std::chrono::system_clock::now();
}


void CameraHandler::record()
{
    m_RecordedSeconds = std::chrono::system_clock::now() - m_RecordingStart;

    if (m_RecordedFrames + 1 > m_SessionParams.FrameLimit && m_SessionParams.LimitFrames) {
        stopRecording();
        return;
    }

    if (m_RecordedSeconds.count() > m_SessionParams.TimeLimitInS && m_SessionParams.LimitTime) {
        stopRecording();
        return;
    }

    m_RecordedFrames += 1;
        
    if (m_SessionParams.EstimateSkeleton) {
        m_SkeletonDetectorNuitrack->update(m_RecordedSeconds.count());
    }
    else {
        if (m_SessionParams.StreamWhileRecording) {
            mp_PointCloud->OnUpdate();
            mp_PointCloud->OnRender();
        }
        else {
            std::for_each(
                std::execution::par,
                m_DepthCameras.begin(),
                m_DepthCameras.end(),
                [](auto&& cam)
                {
                    cam->saveFrame();
                }
            );
        }
    }
}

void CameraHandler::showRecordingStats() {
    ImGui::Begin("Session Stats");

    ImGui::Text("Elapsed Seconds: %.2f s", m_RecordedSeconds.count());
    ImGui::Text("Elapsed Frames: %d", m_RecordedFrames);

    if (m_SessionParams.LimitFrames && m_SessionParams.FrameLimit > 0) {
        ImGui::Text("Frame Limit:");
        ImGui::ProgressBar((float)m_RecordedFrames / (float)m_SessionParams.FrameLimit);
    }

    if (m_SessionParams.LimitTime && m_SessionParams.TimeLimitInS > 0) {
        ImGui::Text("Time Limit:");
        ImGui::ProgressBar(m_RecordedSeconds.count() / (float)m_SessionParams.TimeLimitInS);
    }

    if (ImGui::Button("Stop Recording")) {
        stopRecording();
    }

    ImGui::End();
}

#pragma warning(disable : 4996)
void CameraHandler::stopRecording() {
    if (m_State != Recording)
        return;

    mp_Logger->log("Stopping recording");

    m_RecordingEnd = std::chrono::system_clock::now();

    std::time_t end_time = std::chrono::system_clock::to_time_t(m_RecordingEnd);
    mp_Logger->log("Finished recording at " + (std::string)std::ctime(&end_time) + "elapsed time: " + std::to_string(m_RecordedSeconds.count()) + "s");

    auto configPath = m_RecordingDirectory / getFileSafeSessionName(m_SessionName);
    configPath += ".json";

    std::fstream configJson(configPath, std::ios::out | std::ios::trunc);
    Json::Value root;

    root["Name"] = m_SessionName;
    root["Duration"] = m_RecordedSeconds.count();
    root["Frames"] = m_RecordedFrames;

    Json::Value cameras;

    if (!m_SessionParams.EstimateSkeleton) {
        for (int cam_id = 0; cam_id < m_DepthCameras.size(); cam_id++) {
            auto cam = m_DepthCameras[cam_id];
            if (cam->m_IsSelectedForRecording) {
                auto cam_json = cam->getCameraConfig();
                cam->stopRecording();
                cameras.append(cam_json);
            }
        }
    }
    else {
        cameras.append(m_SkeletonDetectorNuitrack->getCameraJson());
        root["Skeleton"] = m_SkeletonDetectorNuitrack->stopRecording();
    }
    root["Cameras"] = cameras;

    if (m_DepthCameras.size() > 1) {
        root["Rotation"] = mp_PointCloud->getRotation();
        root["Translation"] = mp_PointCloud->getTranslation();
    }

    root["Session Parameters"] = (Json::Value)m_SessionParams;

    Json::StreamWriterBuilder builder;
    configJson << Json::writeString(builder, root);
    configJson.close();

    if (m_SessionParams.EstimateSkeleton) {
        m_SkeletonDetectorNuitrack->freeCameras();
    }

    updateSessionName();
    findRecordings();

    if (!m_SessionParams.EstimateSkeleton) {
        clearCameras();

        initAllCameras();
    }

    if (!m_SessionParams.stopRecording(std::chrono::system_clock::now() - m_RecordingStart))
        initRecording();
    else
        m_State = Streaming;
}

/// 
/// Playback
/// 

void CameraHandler::findRecordings() {
    m_Recordings.clear();

    for (const auto& entry : std::filesystem::directory_iterator(m_RecordingDirectory))
    {
        if (entry.is_regular_file() &&
            entry.path().extension() == ".json" &&
            entry.path().filename().string().find("Skeleton")  == std::string::npos &&
            entry.path().filename().string().find("Exercises") == std::string::npos &&
            entry.path().filename().string().find("Errors")    == std::string::npos) {
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

    std::reverse(m_Recordings.begin(), m_Recordings.end());

    mp_Logger->log("Found " + std::to_string(m_Recordings.size()) + " Recordings in '" + m_RecordingDirectory.generic_string() + "'");
}

void CameraHandler::showRecordings() {
    ImGui::Begin("Recordings");
    static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
    if (ImGui::BeginTable("Recordings", 9, flags)) {
        ImGui::TableSetupColumn("Cameras");
        ImGui::TableSetupColumn("Exercise");
        ImGui::TableSetupColumn("Date");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Frames");
        ImGui::TableSetupColumn("Seconds");
        ImGui::TableSetupColumn("Calculate Skeleton");
        ImGui::TableSetupColumn("Fix Skeleton");
        ImGui::TableSetupColumn("Playback");
        ImGui::TableHeadersRow();

        for (auto recording : m_Recordings) {
            ImGui::TableNextRow();
            bool isValid = false;
            bool isNuitrack = false;
            std::string cams = "";
            for (auto camera : recording["Cameras"]) {
                if (cams != "") {
                    cams += ", ";
                }

                cams += camera["Type"].asString();
                isValid = camera["Type"].asString() == NuiPlaybackCamera::getType() ||
                    camera["Type"].asString() == OrbbecCamera::getType() ||
                    camera["Type"].asString() == RealSenseCamera::getType();
            }

            ImGui::BeginDisabled(!isValid);
            // Cameras
            ImGui::TableSetColumnIndex(0);
            ImGui::Text(cams.c_str());
            // Exercise
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(recording["Session Parameters"]["Exercise"].asCString());
            // Date
            std::string date = recording["Name"].asString().substr(7);
            std::string time = date.substr(date.find("T") + 1);
            date = date.substr(0, date.find("T"));

            ImGui::TableSetColumnIndex(2);
            ImGui::Text(date.c_str());
            // Time
            ImGui::TableSetColumnIndex(3);
            ImGui::Text(time.c_str());
            // Frames
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%d", recording["Frames"].asInt());
            // Seconds
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.2fs", recording["Duration"].asFloat());
            // Skeleton
            ImGui::TableSetColumnIndex(6);
            if (!isNuitrack && ImGui::Button("Calculate")) {
                calculateSkeletonsOpenpose(recording);
            }
            // Fix Skeleton
            ImGui::TableSetColumnIndex(7);
            ImGui::BeginDisabled(recording["Skeleton"].isNull());
            if (ImGui::Button(("Fix##" + recording["Name"].asString()).c_str())) {
                m_FixSkeleton = true;
                startPlayback(recording);
            }
            ImGui::EndDisabled();
            // PLayback
            ImGui::TableSetColumnIndex(8);
            if (ImGui::Button(("Play##" + recording["Name"].asString()).c_str())) {
                startPlayback(recording);
            }
            ImGui::EndDisabled();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void CameraHandler::startPlayback(Json::Value recording)
{
    mp_Logger->log("Started Playback of Recording \"" + recording["Name"].asString() + "\"");

    clearCameras();

    m_FoundRecordedSkeleton = false;
    stopPlayback();
    m_State = Playback;

    m_Recording = recording;

    for (auto camera : recording["Cameras"]) {
        auto rec_dir = (m_RecordingDirectory / camera["FileName"].asCString());
        if (camera["Type"].asString() == RealSenseCamera::getType()) {
            m_DepthCameras.push_back(new RealSenseCamera(mp_Camera, mp_Renderer, mp_Logger, rec_dir, &m_CurrentPlaybackFrame));
        }
        else if (camera["Type"].asString() == OrbbecCamera::getType()) {
            m_DepthCameras.push_back(new OrbbecCamera(mp_Camera, mp_Renderer, mp_Logger, rec_dir, &m_CurrentPlaybackFrame));
        }
        else if (camera["Type"].asString() == NuiPlaybackCamera::getType()) {
            m_DepthCameras.push_back(new NuiPlaybackCamera(mp_Camera, mp_Renderer, mp_Logger, rec_dir, &m_CurrentPlaybackFrame, camera));
        }
        else {
            mp_Logger->log("Camera Type '" + camera["Type"].asString() + "' unknown", Logger::Priority::WARN);
        }
    }

    if (!recording["Skeleton"].isNull()) {
        mp_Logger->log("Skeleton found in '" + (m_RecordingDirectory / recording["Skeleton"].asString()).string() + "'");

        std::ifstream configJson(m_RecordingDirectory / recording["Skeleton"].asString());
        Json::Value root;

        Json::CharReaderBuilder builder;

        builder["collectComments"] = true;

        JSONCPP_STRING errs;

        if (!parseFromStream(builder, configJson, &root, &errs)) {
            mp_Logger->log(errs, Logger::Priority::ERR);
        }
        else {
            mp_Logger->log("Found Skeleton!");

            m_FoundRecordedSkeleton = true;
            m_RecordedSkeleton = root;
        }
        configJson.close();
    }
    else {
        mp_Logger->log("No Skeleton Found for selected recording!", Logger::Priority::WARN);
    }
    
    m_CurrentColorFrame = cv::Mat{};
    m_CurrentPlaybackFrame = 0;
    m_TotalPlaybackFrames = recording["Frames"].asInt();
    m_CamerasExist = !m_DepthCameras.empty();
    if (m_CamerasExist)
        mp_PointCloud = std::make_unique<GLObject::PointCloud>(m_DepthCameras, mp_Camera, mp_Logger, mp_Renderer);
}

void CameraHandler::playback()
{
    if (m_FixSkeleton && m_FoundRecordedSkeleton) {
        fixSkeleton();
    }
    else {
        if (!m_PlaybackPaused) {
            mp_PointCloud->OnUpdate();
        }
        mp_PointCloud->OnRender();
        for (auto cam : m_DepthCameras)
        {
            if (!m_PlaybackPaused && (m_ShowColorFrames || m_DoSkeletonDetection)) {
                auto frame = cam->getColorFrame();
                if (!frame.empty()) {
                    if (m_DoSkeletonDetection) {
                        m_SkeletonDetectorOpenPose->drawSkeleton(frame, m_ScoreThreshold, m_ShowUncertainty);
                    }
                    ImGui::Begin((cam->getCameraName() + (std::string)" Color Frame").c_str());
                    ImGuiHelper::showImage(frame);
                    ImGui::End();
                }
            }
        }
        if (!m_PlaybackPaused) {
            m_CurrentPlaybackFrame = (m_CurrentPlaybackFrame + 1) % m_TotalPlaybackFrames;
        }
    }
}

void CameraHandler::showPlaybackGui()
{
    ImGui::Begin("Playback");

    if (m_State == Playback) {
        ImGui::Checkbox("Pause Playback", &m_PlaybackPaused);
    }

    if (m_State == Playback && ImGui::Button("Stop Playback")) {
        mp_Logger->log("Stopping Playback");
        m_State = Streaming;
        clearCameras();
    }

    if (ImGui::Checkbox("Fix Skeleton", &m_FixSkeleton)) {
        m_CurrentPlaybackFrame = 0;
    }

    if (ImGui::Button("Refresh Recordings")) {
        findRecordings();
    }

    if (ImGui::Button("Convert Recordings")) {
        convertRecordings(m_Recordings, true);
    }

    if (m_Recordings.empty()) {
        ImGui::Text("No Recordings Found!");
    }
    else {
        if (ImGui::Button("(Re)Calculate Skeleton for all recordings")) {
            mp_Logger->log("Starting skeleton detection for " + std::to_string(m_Recordings.size()) + " Recordings");
            int i = 0;
            for (auto rec : m_Recordings) {
                std::cout << i++ << "/" << m_Recordings.size() << "\r";
                calculateSkeletonsOpenpose(rec);
            }

            mp_Logger->log("Skeleton detection done for all recordings");
        }

        if (m_State == Playback) {
            ImGui::ProgressBar((float)m_CurrentPlaybackFrame / (float)m_TotalPlaybackFrames);
        }
    }

    ImGui::End();

    showRecordings();
}

void CameraHandler::fixSkeleton() {
    // TODO: This wont work if there are multiple cameras so we only use the first
    auto cam = m_DepthCameras[0];

    ImGui::Begin("Fix Skeleton", &m_FixSkeleton);

    ImGui::ProgressBar((float)m_CurrentPlaybackFrame / (float)m_TotalPlaybackFrames);
    ImGui::InputInt("Frame", &m_CurrentPlaybackFrame, 1);

    if (m_CurrentPlaybackFrame < 0) {
        m_CurrentPlaybackFrame = m_TotalPlaybackFrames - 1;
    } 
    else if (m_CurrentPlaybackFrame >= m_TotalPlaybackFrames) {
        m_CurrentPlaybackFrame = 0;
    }

    Json::Value& skel_frame = m_RecordedSkeleton[m_CurrentPlaybackFrame * 10];

    if (!skel_frame) {
        m_CurrentPlaybackFrame += 1;
        if (m_CurrentPlaybackFrame == m_TotalPlaybackFrames) {
            stopPlayback();
        }
        return;
    }

    int valid_people = 0;

    mp_PointCloud->OnUpdate();
    m_CurrentColorFrame = cam->getColorFrame();
    if (m_CurrentColorFrame.empty()) {
        mp_Logger->log("No color frame!", Logger::Priority::ERR);
        return;
    }

    for (int i = 0; i < skel_frame.size(); i++){
        Json::Value& person = skel_frame[i];
        bool person_valid = person["error"].asInt() == 0;
        if (ImGui::Checkbox(((std::string)"Person No " + std::to_string(i)).c_str(), &person_valid)) {
            if (person_valid) {
                person["error"] = 0;
            }
            else {
                person["error"] = 1;
            }
        }
                
        if (!person_valid) {
            ImGui::SameLine();
            int err = person["error"].asInt();
            SkeltonErrors.Slider(&err, person["Index"].asInt());
            person["error"] = err;
        }

        ImGui::BeginDisabled(!person_valid);

        for (Json::Value& joint : person["Skeleton"]) {
            const auto score = joint["score"].asDouble();

            bool joint_valid = joint["error"].asInt() == 0;
            auto joint_i = joint["i"].asInt();

            auto checkbox_name = (std::string)"  " + SkeletonDetectorNuitrack::getJointName(joint_i);
            auto checkbox_id = (std::string)"##C" + std::to_string(i) + (std::string)"." + joint["i"].asString();
            auto joint_col = SkeletonDetectorNuitrack::getJointColor(joint_i);

            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(joint_col[2] * 255, joint_col[1] * 255, joint_col[0] * 255, 255));

            if (ImGui::Checkbox((checkbox_name + checkbox_id).c_str(), &joint_valid)) {
                if (joint_valid) {
                    joint["error"] = 0;
                }
                else {
                    joint["error"] = 2;
                }
            }
            ImGui::PopStyleColor();

            if (!joint_valid) {
                ImGui::SameLine();
                int err = joint["error"].asInt();
                JointErrors.Slider(&err, person["Index"].asInt(), joint["i"].asInt());
                joint["error"] = err;
            }


            bool isValid = joint_valid && person_valid;
            cv::Scalar color{ 1.0, 0.0, 0.0, 0.75 };

            if (m_ScoreThreshold < score || m_ShowUncertainty) {
                if (!isValid) {
                    color = { 0, 0, 0, 0.75 };
                }
                else {
                    color = joint_col;
                }
                cv::circle(m_CurrentColorFrame, { joint["u"].asInt(), joint["v"].asInt(), }, 5, color * 255.0f, cv::FILLED);
            }
        }

        ImGui::EndDisabled();
    }

    if (ImGui::Button("Continue")) {
        // Continuing saves
        auto configPath = m_RecordingDirectory / m_Recording["Skeleton"].asString();

        std::fstream configJson(configPath, std::ios::out);

        Json::StreamWriterBuilder builder;
        configJson << Json::writeString(builder, m_RecordedSkeleton);
        configJson.close();

        m_CurrentPlaybackFrame += 1;
        if (m_CurrentPlaybackFrame == m_TotalPlaybackFrames) {
            stopPlayback();
            return;
        }
        mp_PointCloud->OnUpdate();
        m_CurrentColorFrame = cam->getColorFrame();
        if (m_CurrentColorFrame.empty()) {
            mp_Logger->log("No color frame!", Logger::Priority::ERR);
            return;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Save")) {
        auto configPath = m_RecordingDirectory / m_Recording["Skeleton"].asString();

        std::fstream configJson(configPath, std::ios::out);

        Json::StreamWriterBuilder builder;
        configJson << Json::writeString(builder, m_RecordedSkeleton);
        configJson.close();
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop")) {
        stopPlayback();
        return;
    }

    ImGui::End();

    mp_PointCloud->OnRender();

    ImGui::Begin("Skeleton Fix Color Frame");
    ImGuiHelper::showImage(m_CurrentColorFrame);
    ImGui::End();
}

void CameraHandler::stopPlayback()
{
    if (m_FoundRecordedSkeleton) {
        auto configPath = m_RecordingDirectory / m_Recording["Skeleton"].asString();

        std::fstream configJson(configPath, std::ios::out);

        Json::StreamWriterBuilder builder;
        configJson << Json::writeString(builder, m_RecordedSkeleton);
        configJson.close();
        
    }
    m_FoundRecordedSkeleton = false;

    clearCameras();
    m_State = Streaming;
}


/// 
/// Skeleton Calculator (kind of depricated)
/// 

void CameraHandler::calculateSkeletonsNuitrack(Json::Value recording) {
    m_TotalPlaybackFrames = recording["RecordedFrames"].asInt();

    for (auto camera : recording["Cameras"]) {
        if (camera["Type"].asString() != RealSenseCamera::getType() &&
            camera["Type"].asString() != OrbbecCamera::getType()) {
            mp_Logger->log("Camera Type '" + camera["Type"].asString() + "' unknown", Logger::Priority::ERR);
            return;
        }

        m_SkeletonDetectorNuitrack = std::make_unique<SkeletonDetectorNuitrack>(mp_Logger, camera["FileName"].asString(), camera["Type"].asString());
        m_SkeletonDetectorNuitrack->startRecording(getFileSafeSessionName(recording["Name"].asString()));
        m_CurrentPlaybackFrame = 0;

        for (; m_CurrentPlaybackFrame < m_TotalPlaybackFrames; m_CurrentPlaybackFrame++) {
            m_SkeletonDetectorNuitrack->update(m_RecordedSeconds.count());
            std::cout << m_CurrentPlaybackFrame << "/" << m_TotalPlaybackFrames << " Processed" << "\r";
        }

        recording["Skeleton"] = getFileSafeSessionName(recording["Name"].asString()) + "/" + m_SkeletonDetectorNuitrack->stopRecording();
    }

    auto configPath = m_RecordingDirectory / recording["Name"].asString();
    configPath += ".json";

    remove(configPath);

    std::fstream configJson(configPath, std::ios::out | std::ios::trunc);

    Json::StreamWriterBuilder builder;
    configJson << Json::writeString(builder, recording);
    configJson.close();

    updateSessionName();
    findRecordings();

    clearCameras();

    initAllCameras();

    m_State = Streaming;
}

void CameraHandler::calculateSkeletonsOpenpose(Json::Value recording) {
    startPlayback(recording);

    if (!m_CamerasExist) {
        mp_Logger->log("No cameras could be initialised for recording \"" + recording["Name"].asString() + "\"", Logger::Priority::ERR);
        return;
    }

    m_SkeletonDetectorOpenPose->startRecording(getFileSafeSessionName(recording["Name"].asString()));

    for (;m_CurrentPlaybackFrame < m_TotalPlaybackFrames; m_CurrentPlaybackFrame++) {
        for (auto cam : m_DepthCameras) {
            // Get it and forget it
            mp_PointCloud->OnUpdate();
            auto frame_to_process = cam->getColorFrame();
            m_SkeletonDetectorOpenPose->saveFrame(frame_to_process);
        }
        std::cout << m_CurrentPlaybackFrame << "/" << m_TotalPlaybackFrames << " Processed" << "\r";
    }

    recording["Skeleton"] = getFileSafeSessionName(recording["Name"].asString()) + "/" + m_SkeletonDetectorOpenPose->stopRecording();

    auto configPath = m_RecordingDirectory / getFileSafeSessionName(recording["Name"].asString());
    configPath += ".json";
    
    std::filesystem::remove(configPath);
    
    std::fstream configJson(configPath, std::ios::out | std::ios::trunc);

    Json::StreamWriterBuilder builder;
    configJson << Json::writeString(builder, recording);
    configJson.close();
    
    updateSessionName();
    findRecordings();

    clearCameras();

    initAllCameras();

    m_State = Streaming;
}

/// 
/// Utils
/// 

void CameraHandler::clearCameras() {
    for (auto cam : m_DepthCameras)
        delete cam;

    m_DepthCameras.clear();
    mp_PointCloud.release();
    m_CamerasExist = false;
}

void CameraHandler::updateSessionName() {
    auto tp = std::chrono::system_clock::now();
    static auto const CET = std::chrono::locate_zone("Etc/GMT-1");
    m_SessionName = "Session " + std::format("{:%FT%T}", std::chrono::zoned_time{ CET, floor<std::chrono::seconds>(tp) });
}