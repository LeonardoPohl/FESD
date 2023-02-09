#pragma once
#include <filesystem>

#include <nuitrack/Nuitrack.h>
#include <opencv2/opencv.hpp>
#include <json/json.h>

#include "Logger.h"

class SkeletonDetectorNuitrack
{
public:
	SkeletonDetectorNuitrack(Logger::Logger* logger, std::string recordingPath, std::string camera_type);
	~SkeletonDetectorNuitrack();

    void update();

	void startRecording(std::string sessionName);
	std::string stopRecording();
private:
	Logger::Logger* mp_Logger;

	std::filesystem::path m_RecordingPath;

	Json::Value m_Skeletons{ };

    // Skeleton Tracker
    tdv::nuitrack::SkeletonTracker::Ptr m_SkeletonTracker;
    tdv::nuitrack::SkeletonData::Ptr m_SkeletonData;
};

