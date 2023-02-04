#include "SkeletonDetectorNuitrack.h"

#include <fstream>

#include "cameras/OrbbecCamera.h"
#include "cameras/RealsenseCamera.h"
#include "utilities/Consts.h"
#include "utilities/Utils.h"

SkeletonDetectorNuitrack::SkeletonDetectorNuitrack(Logger::Logger* logger, std::string recordingPath, std::string camera_type) : mp_Logger(logger)
{
	mp_Logger->log("Initialising Nuitrack");
	tdv::nuitrack::Nuitrack::init();
	if (camera_type == RealSenseCamera::getType())
		tdv::nuitrack::Nuitrack::setConfigValue("Realsense2Module.FileRecord", (m_RecordingDirectory / recordingPath).string());
	else if (camera_type == OrbbecCamera::getType())
		tdv::nuitrack::Nuitrack::setConfigValue("OrbbecAstra2Module.FileRecord", (m_RecordingDirectory / recordingPath).string());

	// Create Tracker
	m_SkeletonTracker = tdv::nuitrack::SkeletonTracker::create();

	tdv::nuitrack::Nuitrack::run();
}

SkeletonDetectorNuitrack::~SkeletonDetectorNuitrack()
{
	tdv::nuitrack::Nuitrack::release();
}

void SkeletonDetectorNuitrack::update() {
	tdv::nuitrack::Nuitrack::update();

	// Update Tracker
	try {
		tdv::nuitrack::Nuitrack::waitUpdate(m_SkeletonTracker);
	}
	catch (const tdv::nuitrack::LicenseNotAcquiredException& ex) {
		throw std::runtime_error("failed license not acquired");
	}

	// Retrieve Skeleton Data
	m_SkeletonData = m_SkeletonTracker->getSkeletons();

	const std::vector<tdv::nuitrack::Skeleton> skeletons = m_SkeletonData->getSkeletons();

	Json::Value people;
	for (const tdv::nuitrack::Skeleton& skeleton : skeletons) {
		Json::Value person_json;
		Json::Value skeleton_json;
		person_json["Index"] = skeleton.id;
		const std::vector<tdv::nuitrack::Joint> joints = skeleton.joints;
		for (const tdv::nuitrack::Joint& joint : joints) {
			Json::Value joint_json;

			joint_json["i"] = joint.type;
			
			joint_json["u"] = joint.proj.x;
			joint_json["v"] = joint.proj.y;
			
			joint_json["x"] = joint.proj.x;
			joint_json["y"] = joint.proj.y;
			joint_json["z"] = joint.proj.z;
			
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

void SkeletonDetectorNuitrack::stopRecording()
{
	std::fstream configJson(m_RecordingPath, std::ios::out);
	Json::Value root;
	root["Skeletons"] = m_Skeletons;
	Json::StreamWriterBuilder builder;
	configJson << Json::writeString(builder, m_Skeletons);
	configJson.close();
}
