#include "SkeletonDetectorNuitrack.h"

#include "utilities/Consts.h"
#include "utilities/Utils.h"

using namespace tdv::nuitrack;

SkeletonDetectorNuitrack::SkeletonDetectorNuitrack(Logger::Logger* logger, float meters_per_unit, glm::mat3 intrinsics) : mp_Logger(logger), m_MetersPerUnit(meters_per_unit), m_Intrinsics(intrinsics)
{
	mp_Logger->log("Initialising Nuitrack");
	Nuitrack::init("nuitrack/nuitrack.config");

	auto devices = Nuitrack::getDeviceList();
	Nuitrack::setDevice(devices[0]);

	// Create Tracker
	m_ColorSensor = ColorSensor::create();
	m_DepthSensor = DepthSensor::create();
	m_SkeletonTracker = SkeletonTracker::create();
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

Json::Value SkeletonDetectorNuitrack::getCameraJson()
{
	Json::Value camera;
	camera["Type"] = "NuiPlayback";

	camera["Fx"] = m_Intrinsics[0][0];
	camera["Fy"] = m_Intrinsics[1][1];
	camera["Cx"] = m_Intrinsics[0][2];
	camera["Cy"] = m_Intrinsics[1][2];

	camera["MetersPerUnit"] = 1;

	return camera;
}

bool SkeletonDetectorNuitrack::startRecording(std::string sessionName)
{
	try {
		Nuitrack::run();
	}
	catch (const Exception& e) {
		mp_Logger->log("Error running Nuitrack: " + std::to_string(*e.what()), Logger::Priority::ERR);
		return false;
	}

	m_RecordingPath = m_RecordingDirectory / sessionName / "Recordings";

	std::filesystem::create_directory(m_RecordingPath);

	m_CSVRec = std::fstream{ m_RecordingDirectory / sessionName / "Recording.csv", std::ios::out };
	m_CSVRec << "frame_index,timestamp" << std::endl;
	m_Frame = 0;
	//update(0.0, false);

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
	cv::imshow("Data", colorMat);
	colorMat.convertTo(colorMat, CV_16FC3); // Convert to Float matrix to enable merging
	colorMat /= 255.0; // Normalise values to be between 0 and 1
	std::vector<cv::Mat> channels;
	cv::split(colorMat, channels);
	
	cv::Mat depthMat(cv::Size{ depthFrame->getCols(), depthFrame->getRows() }, CV_16UC1, depth);
	depthMat.convertTo(depthMat, CV_16FC1);
	std::cout << depthMat.type();
	depthMat *= m_MetersPerUnit; // Convert units to meters
	depthMat /= 8.0; // Convert to an eight meters per unit to get values between 0 and 1
	channels.push_back(depthMat);
	
	auto frame_num = std::to_string(m_Frame);

	cv::Mat fin;
	cv::merge(channels, fin);
	fin.convertTo(fin, CV_16F);
	if (save) {
		try{
			std::cout << std::endl << fin.depth() << std::endl;
			cv::FileStorage file((m_RecordingPath / (frame_num + ".yml")).string(), cv::FileStorage::WRITE);
			file.write("Frame", fin);
			file.release();
		}
		catch (cv::Exception& e)
		{
			mp_Logger->log(e.msg, Logger::Priority::ERR);
			return false;
		}
	}

	// Retrieve Skeleton Data	
	const std::vector<Skeleton> skeletons = m_SkeletonTracker->getSkeletons()->getSkeletons();

	Json::Value people{};
	for (const Skeleton& skeleton : skeletons) {
		Json::Value person_json;
		Json::Value skeleton_json;
		person_json["id"] = skeleton.id;
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

	if (save) {
		std::fstream configJson(m_RecordingPath / (frame_num + ".json"), std::ios::out);
		Json::StreamWriterBuilder builder;
		configJson << Json::writeString(builder, people);
		configJson.close();

		m_CSVRec << frame_num << "," << time_stamp << std::endl;
		m_Frame += 1;
	}

	return true;
}

std::string SkeletonDetectorNuitrack::stopRecording()
{
	m_CSVRec.close();
	return m_RecordingPath.filename().string();
}
