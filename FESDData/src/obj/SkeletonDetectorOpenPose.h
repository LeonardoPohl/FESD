#pragma once
#include <filesystem>

#include <openpose/headers.hpp>
#include <openpose/filestream/fileStream.hpp>
#include <openpose/pose/poseParameters.hpp>
#include <opencv2/opencv.hpp>
#include <json/json.h>

#include "Logger.h"

class SkeletonDetectorOpenPose
{
public:
	SkeletonDetectorOpenPose(Logger::Logger *logger);

	op::Array<float> calculateSkeleton(cv::Mat frame_to_process);
	void drawSkeleton(cv::Mat &frame_to_process, float score_threshold = 0.0f, bool show_uncertainty = false);

	void startRecording(std::string sessionName);
	void saveFrame(cv::Mat frame_to_process);
	void stopRecording();
private:
	Logger::Logger *mp_Logger;

	op::Wrapper m_OPWrapper{ op::ThreadManagerMode::Asynchronous };
	std::filesystem::path m_RecordingPath;

	Json::Value m_Skeletons{ };
};

