#include "SkeletonDetectorNuitrack.h"

#include <vector>

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


std::string SkeletonDetectorNuitrack::getJointName(int joint_id)
{
	std::vector<std::string> joint_names{ "-", "Head", "Neck", "Torso", "Waist", "Left collar", "Left shoulder", "Left elbow", "Left wrist", "Left hand", "-", "Right collar", "Right shoulder", "Right elbow", "Right wrist", "Right hand", "-", "Left hip", "Left knee", "Left ankle", "-", "Right hip", "Right knee", "Right ankle", "-" };

	return joint_names[joint_id];
}

cv::Scalar SkeletonDetectorNuitrack::getJointColor(int joint_id)
{
	std::vector<cv::Scalar> joint_color{ {0.000, 0.000, 0.000}, {0.902, 0.098, 0.294}, {0.235, 0.706, 0.294}, {1.000, 0.882, 0.098}, {0.000, 0.510, 0.784}, {0.961, 0.510, 0.188}, {0.569, 0.118, 0.706}, {0.275, 0.941, 0.941}, {0.941, 0.196, 0.902}, {0.824, 0.961, 0.235}, {0.000, 0.000, 0.000}, {0.980, 0.745, 0.831}, {0.000, 0.502, 0.502}, {0.863, 0.745, 1.000}, {0.667, 0.431, 0.157}, {1.000, 0.980, 0.784}, {0.000, 0.000, 0.000}, {0.502, 0.000, 0.000}, {0.667, 1.000, 0.765}, {0.502, 0.502, 0.000}, {1.000, 0.843, 0.706}, {0.000, 0.000, 0.000}, {0.000, 0.000, 0.502}, {0.502, 0.502, 0.502}, {1.000, 1.000, 1.000}, {1.000, 0.876, 0.513}, {0.000, 0.000, 0.000} };

	return joint_color[joint_id];
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

	camera["FileName"] = (m_FramePath.parent_path().filename() / m_FramePath.filename()).string();

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
	fin.convertTo(fin, CV_16F);
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

std::string SkeletonDetectorNuitrack::stopRecording()
{
	m_CSVRec.close();

	for (int i = 0; i < m_Frames.size(); i++) {
		std::cout << i << "/" << m_Frames.size() << " Frames stored!\r";
		std::ofstream fs((m_FramePath / (getFrameName(i) + ".bin")).string(), std::fstream::binary);

		// Header
		int type = m_Frames[i].type();
		int channels = m_Frames[i].channels();
		fs.write((char*)&m_Frames[i].rows, sizeof(int));    // rows
		fs.write((char*)&m_Frames[i].cols, sizeof(int));    // cols
		fs.write((char*)&type, sizeof(int));				// type
		fs.write((char*)&channels, sizeof(int));    // channels

		// Data
		if (m_Frames[i].isContinuous())
		{
			fs.write(m_Frames[i].ptr<char>(0), (m_Frames[i].dataend - m_Frames[i].datastart));
		}
		else
		{
			int rowsz = CV_ELEM_SIZE(type) * m_Frames[i].cols;
			for (int r = 0; r < m_Frames[i].rows; ++r)
			{
				fs.write(m_Frames[i].ptr<char>(r), rowsz);
			}
		}
		/*cv::FileStorage frameStorage ((m_FramePath / (getFrameName(i) + ".yml")).string(), cv::FileStorage::WRITE);
		try {
			frameStorage.write("frame", m_Frames[i]);
		}
		catch (cv::Exception& e)
		{
			mp_Logger->log(e.msg, Logger::Priority::ERR);
		}
		frameStorage.release();*/
	}
	m_Frames.clear();

	mp_Logger->log("All frames stored!");

	std::fstream configJson(m_RecordingPath / "SkeletonNui.json", std::ios::out | std::ios::trunc);
	Json::StreamWriterBuilder builder;
	configJson << Json::writeString(builder, m_Skeletons);
	configJson.close();
	m_Skeletons.clear();

	return (m_RecordingPath.filename() / "SkeletonNui.json").string();
}
