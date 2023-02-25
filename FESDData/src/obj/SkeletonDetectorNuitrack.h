#pragma once
#include <filesystem>
#include <fstream>

#include <glm/glm.hpp>
#include <nuitrack/Nuitrack.h>
#include <opencv2/opencv.hpp>
#include <json/json.h>

#include "Logger.h"

class SkeletonDetectorNuitrack
{
public:
	SkeletonDetectorNuitrack(Logger::Logger* logger, float meters_per_unit, glm::mat3 intrinsics);
	SkeletonDetectorNuitrack(Logger::Logger* logger, std::string recordingPath, std::string camera_type);
	~SkeletonDetectorNuitrack();

	static std::string getFrameName(int frame);
	void freeCameras();

	Json::Value getCameraJson();
	bool startRecording(std::string sessionName);
	bool update(double times_tamp, bool save = true);
	std::string stopRecording();
private:
	Logger::Logger* mp_Logger;

	std::filesystem::path m_RecordingPath;

	int m_Frame{ 0 };
	float m_MetersPerUnit{ };

	std::fstream m_CSVRec{ };
	cv::FileStorage m_FrameStorage{ };
	Json::Value m_Skeletons{ };
	glm::mat3 m_Intrinsics{ };

    // Skeleton Tracker
    tdv::nuitrack::ColorSensor::Ptr m_ColorSensor;
    tdv::nuitrack::DepthSensor::Ptr m_DepthSensor;
    tdv::nuitrack::SkeletonTracker::Ptr m_SkeletonTracker;
    tdv::nuitrack::SkeletonData::Ptr m_SkeletonData;
};

