#include "SkeletonDetectorNuitrack.h"

#include "Error.h"
#include "utilities/Consts.h"
#include "utilities/Utils.h"

using namespace tdv::nuitrack;

SkeletonDetectorNuitrack::SkeletonDetectorNuitrack(Logger::Logger* logger, glm::mat3 intrinsics) : mp_Logger(logger), m_Intrinsics(intrinsics)
{
	mp_Logger->log("Initialising Nuitrack");
	Nuitrack::init("nuitrack/nuitrack.config");
}

SkeletonDetectorNuitrack::SkeletonDetectorNuitrack(Logger::Logger* logger, std::string recordingPath, std::string camera_type) : mp_Logger(logger)
{
	mp_Logger->log("This is not implemented and will result in an error");
	Nuitrack::init();
	/*
	if (camera_type == RealSenseCamera::getType()) {
		Nuitrack::setConfigValue("Realsense2Module.FileRecord", (m_RecordingDirectory / recordingPath).string());
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

std::string SkeletonDetectorNuitrack::getFrameName(int frame)
{
	return "frame_" + std::to_string(frame);
}

void SkeletonDetectorNuitrack::freeCameras()
{
	Nuitrack::release();
}

Json::Value SkeletonDetectorNuitrack::getCameraJson()
{
	Json::Value camera;
	camera["Name"] = "NuiPlayback";
	camera["Type"] = "NuiPlayback";

	camera["Fx"] = m_Intrinsics[0][0];
	camera["Fy"] = m_Intrinsics[1][1];
	camera["Cx"] = m_Intrinsics[0][2];
	camera["Cy"] = m_Intrinsics[1][2];

	camera["MetersPerUnit"] = 1;

	camera["FileName"] = m_FramePath.filename().string();

	return camera;
}

bool SkeletonDetectorNuitrack::startRecording(std::string sessionName)
{
	auto devices = Nuitrack::getDeviceList();
	Nuitrack::setDevice(devices[0]);

	// Create Tracker
	m_ColorSensor = ColorSensor::create();
	m_DepthSensor = DepthSensor::create();
	m_SkeletonTracker = SkeletonTracker::create();

	try {
		Nuitrack::run();
	}
	catch (const Exception& e) {
		mp_Logger->log("Error running Nuitrack: " + std::to_string(*e.what()), Logger::Priority::ERR);
		return false;
	}

	m_RecordingPath = m_RecordingDirectory / sessionName;
	m_FramePath = m_RecordingPath / "Frames";

	std::filesystem::create_directory(m_RecordingPath);
	std::filesystem::create_directory(m_FramePath);

	m_CSVRec = std::fstream{ m_RecordingPath / "Timestamps.csv", std::ios::out };
	m_CSVRec << "frame_index,timestamp" << std::endl;
	m_Frame = 0;

	return true;
}

bool SkeletonDetectorNuitrack::update(double time_stamp, bool save) {	
	// Update Tracker
	try {
		Nuitrack::update();
		Nuitrack::waitUpdate(m_SkeletonTracker);
	}
	catch (const LicenseNotAcquiredException& ex) {
		mp_Logger->log("Nuitrack license not acquired", Logger::Priority::ERR);
		return false;
	}
	catch (const Exception& ex) {
		mp_Logger->log(ex.what(), Logger::Priority::ERR);
		return false;
	}

	auto colorFrame = m_ColorSensor->getColorFrame();
	void* color = (void*)colorFrame->getData();
	auto depthFrame = m_DepthSensor->getDepthFrame();
	void* depth = (void*)depthFrame->getData();

	if (colorFrame == nullptr || depthFrame == nullptr) {
		mp_Logger->log("No Color or depth", Logger::Priority::ERR);
		return false;
	}

	cv::Mat colorMat(cv::Size{ colorFrame->getCols(), colorFrame->getRows() }, CV_8UC3, color);
	colorMat.convertTo(colorMat, CV_16FC3); // Convert to Float matrix to enable merging
	colorMat /= 255.0; // Normalise values to be between 0 and 1
	
	cv::Mat depthMat(cv::Size{ depthFrame->getCols(), depthFrame->getRows() }, CV_16UC1, depth);
	depthMat.convertTo(depthMat, CV_16FC1);
	depthMat /= 1000.f; // Convert units to meters

	std::vector<cv::Mat> channels;
	cv::split(colorMat, channels);
	channels.push_back(depthMat);
	
	cv::Mat fin;
	cv::merge(channels, fin);
	fin.convertTo(fin, CV_32F);
	if (save) {
		m_Frames.push_back(fin);
	}

	// Retrieve Skeleton Data	
	const std::vector<Skeleton> skeletons = m_SkeletonTracker->getSkeletons()->getSkeletons();
	
	Json::Value people{};
	for (const Skeleton& skeleton : skeletons) {
		Json::Value person_json;
		Json::Value skeleton_json;
		person_json["id"] = skeleton.id;
		person_json["error"] = SkeltonErrors[0].id;

		const std::vector<Joint> joints = skeleton.joints;
		for (const Joint& joint : joints) {
			Json::Value joint_json;
			joint_json["error"] = JointErrors[joint.proj.x == 0 && joint.proj.y == 0 ? 1 : 0].id;
			joint_json["i"] = joint.type;
			
			joint_json["u"] = joint.proj.x * colorFrame->getCols();
			joint_json["v"] = joint.proj.y * colorFrame->getRows();
			joint_json["d"] = joint.proj.z / 1000.f;

			// Units are in cm but depth stored in m
			joint_json["x"] = joint.real.x / 1000.f;
			joint_json["y"] = joint.real.y / 1000.f;
			joint_json["z"] = joint.real.z / 1000.f;
			
			joint_json["score"] = joint.confidence;
			
			skeleton_json.append(joint_json);
		}

		person_json["Skeleton"] = skeleton_json;
		people.append(person_json);
	}
	m_Skeletons.append(people);

	if (save) {
		m_CSVRec << m_Frame << "," << time_stamp << std::endl;
		m_Frame += 1;
	}

	return true;
}

std::string SkeletonDetectorNuitrack::stopRecording(bool sound)
{
	m_CSVRec.close();

	for (int i = 0; i < m_Frames.size(); i++) {
		std::cout << i << "/" << m_Frames.size() << " Frames stored!\r";
		cv::FileStorage frameStorage ((m_FramePath / (getFrameName(i) + ".yml")).string(), cv::FileStorage::WRITE);
		try {
			frameStorage.write("frame", m_Frames[i]);
		}
		catch (cv::Exception& e)
		{
			mp_Logger->log(e.msg, Logger::Priority::ERR);
		}
		frameStorage.release();
	}
	m_Frames.clear();

	mp_Logger->log("All frames stored!");

	std::fstream configJson(m_RecordingPath / "SkeletonNui.json", std::ios::out | std::ios::trunc);
	Json::Value root;
	root["Skeletons"] = m_Skeletons;
	Json::StreamWriterBuilder builder;
	configJson << Json::writeString(builder, m_Skeletons);
	configJson.close();

	if (sound) {
		std::cout << "\a";
	}

	return "SkeletonNui.json";
}
