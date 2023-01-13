#include "SkeletonDetector.h"

#define OPENPOSE_FLAGS_DISABLE_POSE
#include <openpose/flags.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

SkeletonDetector::SkeletonDetector(Logger::Logger* logger) : mp_Logger(logger)
{
    // Starting OpenPose
    mp_Logger->log("Starting openpose thread(s)");
    m_OPWrapper.start();
}

op::Array<float> SkeletonDetector::calculateSkeleton(cv::Mat frame_to_process)
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

void SkeletonDetector::drawSkeleton(cv::Mat& frame_to_process, float score_threshold, bool show_uncertainty)
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

void SkeletonDetector::startRecording(std::string sessionName)
{
}

void SkeletonDetector::saveFrame()
{
}

void SkeletonDetector::stopRecording()
{
}
