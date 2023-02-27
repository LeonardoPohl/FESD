#include "SkeletonDetectorOpenPose.h"

#define OPENPOSE_FLAGS_DISABLE_POSE
#include <openpose/flags.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Error.h"
#include "utilities/Consts.h"
#include "utilities/Utils.h"

SkeletonDetectorOpenPose::SkeletonDetectorOpenPose(Logger::Logger* logger) : mp_Logger(logger)
{
    // Starting OpenPose
    mp_Logger->log("Starting openpose thread(s)");
    m_OPWrapper.start();
}

op::Array<float> SkeletonDetectorOpenPose::calculateSkeleton(cv::Mat frame_to_process)
{
    const op::Matrix imageToProcess = OP_CV2OPCONSTMAT(frame_to_process);
    auto datumProcessed = m_OPWrapper.emplaceAndPop(imageToProcess);
    if (datumProcessed != nullptr) {
        return datumProcessed->at(0)->poseKeypoints;
        op::Array<float> keyPoints = datumProcessed->at(0)->poseKeypoints;
        for (int i = 0; i < keyPoints.getSize()[0]; i += keyPoints.getStride()[1]) {
            auto kp = keyPoints[0];
        }
    }
    else {
        mp_Logger->log("Frame could not be processed.", Logger::Priority::ERR);
    }
}

void SkeletonDetectorOpenPose::drawSkeleton(cv::Mat& frame_to_process, float score_threshold, bool show_uncertainty)
{
    auto key_points = calculateSkeleton(frame_to_process);

    const auto numberPeopleDetected = key_points.getSize(0);
    const auto numberBodyParts = key_points.getSize(1);

    for (int person = 0; person < numberPeopleDetected; person++) {
        for (int part = 0; part < numberBodyParts; part++) {
            const auto x = key_points[{person, part, 0}];
            const auto y = key_points[{person, part, 1}];
            const auto score = key_points[{person, part, 2}];
            cv::Scalar color{ 1.0, 0.0, 0.0 };

            if (score_threshold < score || show_uncertainty) {
                if (show_uncertainty && score_threshold < score) {
                    color = { 0.3, 0.3, 0.3 };
                }
                else {
                    color = { (0.7 * (score - score_threshold) / (1.0 - score_threshold)) + 0.3, 0.3, 0.3 };
                }
                cv::circle(frame_to_process, { (int)x, (int)y }, 5, color * 255.0f, cv::FILLED);
            }
        }
    }
}

void SkeletonDetectorOpenPose::startRecording(std::string sessionName)
{
    m_RecordingPath = m_RecordingDirectory / sessionName / "OPSkeleton.json";
    m_Skeletons.clear();
}

void SkeletonDetectorOpenPose::saveFrame(cv::Mat frame_to_process)
{
    auto key_points = calculateSkeleton(frame_to_process);

    const auto numberPeopleDetected = key_points.getSize(0);
    const auto numberBodyParts = key_points.getSize(1);

    Json::Value people;
    for (int person = 0; person < numberPeopleDetected; person++) {
        Json::Value p;
        Json::Value skeleton;
        p["Index"] = person;
        p["valid"] = true;

        for (int part = 0; part < numberBodyParts; part++) {
            const auto x = key_points[{person, part, 0}];
            const auto y = key_points[{person, part, 1}];
            const auto score = key_points[{person, part, 2}];
            Json::Value joint;
            joint["i"] = part;
            joint["valid"] = true;

            joint["u"] = key_points[{person, part, 0}];
            joint["v"] = key_points[{person, part, 1}];
            joint["score"] = key_points[{person, part, 2}];
            skeleton.append(joint);
        }
        p["Skeleton"] = skeleton;
        people.append(p);
    }
    m_Skeletons.append(people);
}

std::string SkeletonDetectorOpenPose::stopRecording()
{
    std::fstream configJson(m_RecordingPath, std::ios::out | std::ios::trunc);
    Json::Value root;
    root["Skeletons"] = m_Skeletons;
    Json::StreamWriterBuilder builder;
    configJson << Json::writeString(builder, m_Skeletons);
    configJson.close();
    return m_RecordingPath.filename().string();
}
