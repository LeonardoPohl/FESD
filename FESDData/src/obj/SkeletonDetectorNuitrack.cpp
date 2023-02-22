#include "SkeletonDetectorNuitrack.h"

#include <fstream>

#include "cameras/OrbbecCamera.h"
#include "cameras/RealsenseCamera.h"
#include "utilities/Consts.h"
#include "utilities/Utils.h"

using namespace tdv::nuitrack;

SkeletonDetectorNuitrack::SkeletonDetectorNuitrack(Logger::Logger* logger, std::string recordingPath, std::string camera_type) : mp_Logger(logger)
{
	mp_Logger->log("Initialising Nuitrack");
	Nuitrack::init();
	/*
	if (camera_type == RealSenseCamera::getType()) {
		Nuitrack::setConfigValue("Realsense2Module.Depth.FPS", "30");
		Nuitrack::setConfigValue("Realsense2Module.Depth.RawWidth", "1280");
		Nuitrack::setConfigValue("Realsense2Module.Depth.RawHeight", "720");
		Nuitrack::setConfigValue("Realsense2Module.Depth.ProcessWidth", "1280");
		Nuitrack::setConfigValue("Realsense2Module.Depth.ProcessHeight", "720");
		Nuitrack::setConfigValue("Realsense2Module.Depth.ProcessMaxDepth", "7000");

		Nuitrack::setConfigValue("Realsense2Module.RGB.FPS", "30");
		Nuitrack::setConfigValue("Realsense2Module.RGB.RawWidth", "1280");
		Nuitrack::setConfigValue("Realsense2Module.RGB.RawHeight", "720");
		Nuitrack::setConfigValue("Realsense2Module.RGB.ProcessWidth", "1280");
		Nuitrack::setConfigValue("Realsense2Module.RGB.ProcessHeight", "720");

		// post processing setting
		Nuitrack::setConfigValue("Realsense2Module.Depth.PostProcessing.SpatialFilter.spatial_alpha", "0.1");
		Nuitrack::setConfigValue("Realsense2Module.Depth.PostProcessing.SpatialFilter.spatial_delta", "50");

		Nuitrack::setConfigValue("Realsense2Module.FileRecord", (m_RecordingDirectory / recordingPath).string());
		//Nuitrack::setConfigValue("Realsense2Module.Depth2ColorRegistration", "true");
	}
	else if (camera_type == OrbbecCamera::getType()) {
		Nuitrack::setConfigValue("OpenNIModule.FileRecord", (m_RecordingDirectory / recordingPath).string());
	}
	else {
		return;
	}*/

	auto devices = Nuitrack::getDeviceList();
	Nuitrack::setDevice(devices[0]);
	// Create Tracker
	m_SkeletonTracker = SkeletonTracker::create();

	try {
		Nuitrack::run();
	}
	catch (const Exception& e) {
		mp_Logger->log("Error running Nuitrack: " + std::to_string(*e.what()), Logger::Priority::ERR);
	}
}

SkeletonDetectorNuitrack::~SkeletonDetectorNuitrack()
{
	Nuitrack::release();
}

void SkeletonDetectorNuitrack::update() {
	//Nuitrack::update();

	// Update Tracker
	try {
		Nuitrack::waitUpdate(m_SkeletonTracker);
	}
	catch (const LicenseNotAcquiredException& ex) {
		throw std::runtime_error("failed license not acquired");
	}

	// Retrieve Skeleton Data
	m_SkeletonData = m_SkeletonTracker->getSkeletons();

	const std::vector<Skeleton> skeletons = m_SkeletonData->getSkeletons();

	Json::Value people;
	for (const Skeleton& skeleton : skeletons) {
		Json::Value person_json;
		Json::Value skeleton_json;
		person_json["Index"] = skeleton.id;
		person_json["valid"] = true;

		const std::vector<Joint> joints = skeleton.joints;
		for (const Joint& joint : joints) {
			Json::Value joint_json;
			joint_json["valid"] = true;
			joint_json["i"] = joint.type;
			
			joint_json["u"] = joint.proj.x;
			joint_json["v"] = joint.proj.y;

			joint_json["x"] = joint.real.x;
			joint_json["y"] = joint.real.y;
			joint_json["z"] = joint.real.z;
			
			joint_json["score"] = joint.confidence;
			
			skeleton_json.append(joint_json);
		}

		person_json["Skeleton"] = skeleton_json;
		people.append(person_json);
	}
	m_Skeletons.append(people);
}

void SkeletonDetectorNuitrack::startRecording(std::string sessionName)
{
	m_RecordingPath = m_RecordingDirectory / (getFileSafeSessionName(sessionName) + "_NuiSkeleton.json");
	m_Skeletons.clear();
}

std::string SkeletonDetectorNuitrack::stopRecording()
{
	std::fstream configJson(m_RecordingPath, std::ios::out);
	Json::Value root;
	root["Skeletons"] = m_Skeletons;
	Json::StreamWriterBuilder builder;
	configJson << Json::writeString(builder, m_Skeletons);
	configJson.close();
	return m_RecordingPath.filename().string();
}
