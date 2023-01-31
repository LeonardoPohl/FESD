#include "RealsenseCamera.h"

#include <exception>
#include <iostream>
#include <opencv2/imgproc.hpp>

#include "obj/PointCloud.h"
#include "utilities/Consts.h"

/// 
/// Constructors & Destructors
/// 

RealSenseCamera::RealSenseCamera(rs2::context* ctx, rs2::device* device, Camera* cam, Renderer* renderer, int camera_id, Logger::Logger* logger)
	:
	mp_Context(ctx),
	mp_ProtoDevice(device),
	m_Device(*device),
	mp_Logger(logger)
{
	mp_Pipe = std::make_shared<rs2::pipeline>(*ctx);
	m_CameraId = camera_id;

	printDeviceInfo();
	m_Device.query_sensors();
	m_Config.enable_device(m_Device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

	auto profile = mp_Pipe->start(m_Config);

	rs2::frameset data = mp_Pipe->wait_for_frames(); // Wait for next set of frames from the camera
	rs2::frame depth = data.get_depth_frame();

	// Get Intrinsics
	auto depth_profile = depth.get_profile().as<rs2::video_stream_profile>();
	m_Intrinsics = depth_profile.get_intrinsics();

	// Query frame size (width and height)
	m_DepthWidth = m_Intrinsics.width;
	m_DepthHeight = m_Intrinsics.height;

	rs2::depth_frame depth_frame = depth.as<rs2::depth_frame>();
	m_MetersPerUnit = depth_frame.get_units();
}

RealSenseCamera::RealSenseCamera(Camera* cam, Renderer* renderer, Logger::Logger* logger, std::filesystem::path recording, int *currentPlaybackFrame) :
	mp_Logger(logger), mp_CurrentPlaybackFrame(currentPlaybackFrame)
{
	mp_Pipe = std::make_shared<rs2::pipeline>();
	rs2::config cfg;
	cfg.enable_device_from_file(recording.string());
	mp_Pipe->start(cfg); //File will be opened in read mode at this point
	m_Device = mp_Pipe->get_active_profile().get_device();
	m_Device.as<rs2::playback>().set_real_time(false);

	// Wait for first frame
	rs2::frameset data = mp_Pipe->wait_for_frames();
	rs2::frame depth = data.get_depth_frame();

	// Get Intrinsics
	auto depth_profile = depth.get_profile().as<rs2::video_stream_profile>();
	m_Intrinsics = depth_profile.get_intrinsics();

	// Query frame size (width and height)
	m_DepthWidth = m_Intrinsics.width;
	m_DepthHeight = m_Intrinsics.height;

	rs2::depth_frame depth_frame = depth.as<rs2::depth_frame>();
	m_MetersPerUnit = depth_frame.get_units();
}

RealSenseCamera::~RealSenseCamera() {
	mp_Logger->log("Shutting down [Realsense] " + getCameraName());
	
	try {
		mp_Pipe->stop();
	}
	catch (...) {
		mp_Logger->log("An exception occured while shutting down [Realsense] Camera " + getCameraName());
	}
}


/// 
/// Initialise all devices
/// 

std::vector<RealSenseCamera*> RealSenseCamera::initialiseAllDevices(Camera* cam, Renderer* renderer, int* starting_id, Logger::Logger* logger) {
	rs2::context ctx;

	std::vector<RealSenseCamera*> depthCameras;

	for (auto&& dev : ctx.query_devices())
	{
		depthCameras.push_back(new RealSenseCamera(&ctx, &dev, cam, renderer, (*starting_id)++, logger));
		logger->log("Initialised " + depthCameras.back()->getCameraName());
	}

	return depthCameras;
}


/// 
/// Camera Details
/// 

std::string RealSenseCamera::getType() 
{ 
	return "Realsense"; 
}

inline std::string RealSenseCamera::getCameraName() const
{
	return this->getType() + " Camera " + std::to_string(this->m_CameraId);
}

void RealSenseCamera::showCameraInfo() {
	if (ImGui::TreeNode(getCameraName().c_str())) {
		ImGui::Text("Name: %s", m_Device.get_info(RS2_CAMERA_INFO_NAME));
		ImGui::Text("Produc Line: %s", m_Device.get_info(RS2_CAMERA_INFO_PRODUCT_LINE));
		ImGui::Text("Serial Number: %s", m_Device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
		ImGui::Text("Physical Port: %s", m_Device.get_info(RS2_CAMERA_INFO_PHYSICAL_PORT));
		ImGui::TreePop();
	}
}

void RealSenseCamera::printDeviceInfo() const {
	printf("---\nDevice: %s\n", m_Device.get_info(RS2_CAMERA_INFO_NAME));
	printf("Produc Line: %s\n", m_Device.get_info(RS2_CAMERA_INFO_PRODUCT_LINE));
	printf("Serial Number: %s\n", m_Device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	printf("Physical Port: %s\n\n", m_Device.get_info(RS2_CAMERA_INFO_PHYSICAL_PORT));
}

inline float RealSenseCamera::getIntrinsics(INTRINSICS intrin) const
{
	switch (intrin)
	{
	case INTRINSICS::FX:
		return m_Intrinsics.fx;
	case INTRINSICS::FY:
		return m_Intrinsics.fy;
	case INTRINSICS::CX:
		return m_Intrinsics.ppx;
	case INTRINSICS::CY:
		return m_Intrinsics.ppy;
	default:
		break;
	}
}

inline glm::mat3 RealSenseCamera::getIntrinsics() const
{
	return { m_Intrinsics.fx,		     0.0f, m_Intrinsics.ppx,
						0.0f, m_Intrinsics.fy, m_Intrinsics.ppy,
						0.0f,		     0.0f,             1.0f };
}

inline float RealSenseCamera::getMetersPerUnit() const
{
	return m_MetersPerUnit;
}


/// 
/// Frame retreival
/// 

const void* RealSenseCamera::getDepth()
{
	if (!m_Device.as<rs2::playback>()) {
		rs2::frameset data = mp_Pipe->wait_for_frames(); // Wait for next set of frames from the camera
		rs2::depth_frame depth = data.get_depth_frame();
		if (m_Device.as<rs2::recorder>())
			rs2::video_frame color = data.get_color_frame();
		return depth.get_data();
	}
	else {
		rs2::frameset data;

		if (mp_Pipe->poll_for_frames(&data)) // Check if new frames are ready
		{
			rs2::depth_frame depth = data.get_depth_frame();
			rs2::video_frame color = data.get_color_frame();
			return depth.get_data();
		}

		return nullptr;
	}
}

cv::Mat RealSenseCamera::getColorFrame()
{
	rs2::frameset data;

	if (m_Device.as<rs2::playback>()) {
		if (!mp_Pipe->poll_for_frames(&data)) {
			return {};
		}
	}
	else {
		data = mp_Pipe->wait_for_frames();

	}

	// Make sure the frameset is spatialy aligned 
	// (each pixel in depth image corresponds to the same pixel in the color image)
	rs2::frameset aligned_set = m_AlignToDepth.process(data);
	rs2::video_frame color_frame = aligned_set.get_color_frame();
	cv::Mat color_mat = { cv::Size(color_frame.get_width(), color_frame.get_height()), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP };
	cv::cvtColor(color_mat, color_mat, cv::COLOR_BGR2RGB);
	return color_mat;
}


/// 
/// Recording
/// 

std::string RealSenseCamera::startRecording(std::string sessionName)
{
	// https://dev.intelrealsense.com/docs/rs-record-playback
	auto cameraName = getCameraName();
	std::ranges::replace(cameraName, ' ', '_');

	std::filesystem::path filepath = m_RecordingDirectory / (sessionName + "_" + cameraName + ".bag");
	if (!(m_Device).as<rs2::recorder>())
	{
		mp_Pipe->stop();
		mp_Pipe = std::make_shared<rs2::pipeline>();
		rs2::config cfg;
		mp_Logger->log("Creaded Realsense Recorder");
		
		cfg.enable_record_to_file(filepath.string());
		mp_Pipe->start(cfg);
		m_Device = mp_Pipe->get_active_profile().get_device();
	}

	m_CameraInfromation["Name"] = getCameraName();
	m_CameraInfromation["Type"] = getType();

	m_CameraInfromation["Fx"] = getIntrinsics(INTRINSICS::FX);
	m_CameraInfromation["Fy"] = getIntrinsics(INTRINSICS::FY);
	m_CameraInfromation["Cx"] = getIntrinsics(INTRINSICS::CX);
	m_CameraInfromation["Cy"] = getIntrinsics(INTRINSICS::CY);
	m_CameraInfromation["MeterPerUnit"] =  getMetersPerUnit();

	m_CameraInfromation["FileName"] = filepath.filename().string();

	m_IsSelectedForRecording = true;
	m_IsEnabled = true;

	return filepath.filename().string();
}

void RealSenseCamera::saveFrame() {
	try {
		rs2::frameset data = mp_Pipe->wait_for_frames();
		data.get_depth_frame();
		data.get_color_frame();
	}
	catch (...) {
		mp_Logger->log("Querying realsense frame failed", Logger::Priority::WARN);
	}
}

void RealSenseCamera::stopRecording()
{
	mp_Pipe->stop();
}
