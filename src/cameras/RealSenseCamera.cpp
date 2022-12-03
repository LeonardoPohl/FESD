#include "RealsenseCamera.h"
#include <iostream>
#include <exception>
#include "obj/PointCloud.h"

// TODO: Add depth tuning

rs2::device_list RealSenseCamera::getAvailableDevices(rs2::context ctx) {
	return ctx.query_devices();
}

std::vector<RealSenseCamera*> RealSenseCamera::initialiseAllDevices(Camera* cam, Renderer *renderer, int *starting_id) {
	rs2::context ctx;

	std::vector<RealSenseCamera*> depthCameras;

	for (auto&& dev : RealSenseCamera::getAvailableDevices(ctx))
	{
		depthCameras.push_back(new RealSenseCamera(&ctx, &dev, cam, renderer, (*starting_id)++));
		std::cout << "Initialised " << depthCameras.back()->getCameraName() << std::endl;
	}

	return depthCameras;
}

RealSenseCamera::RealSenseCamera(rs2::context *ctx, rs2::device *device, Camera *cam, Renderer *renderer, int camera_id)
	:
	m_Pipe(rs2::pipeline(*ctx)),
	mp_Context(ctx),
	mp_Device(device)
{
	m_CameraId = camera_id;

	printDeviceInfo();
	mp_Device->query_sensors();
	m_Config.enable_device(mp_Device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	
	auto profile = m_Pipe.start(m_Config);

	rs2::frameset data = m_Pipe.wait_for_frames(); // Wait for next set of frames from the camera
	rs2::frame depth = data.get_depth_frame();

	// Get Intrinsics
	auto depth_profile = depth.get_profile().as<rs2::video_stream_profile>();
	m_Intrinsics = depth_profile.get_intrinsics();
	
	// Query frame size (width and height)
	m_DepthWidth = m_Intrinsics.width;
	m_DepthHeight = m_Intrinsics.height;

	rs2::depth_frame depth_frame = depth.as<rs2::depth_frame>();

	m_PointCloud = std::make_unique<GLObject::PointCloud>(this, cam, renderer, depth_frame.get_units());
}

RealSenseCamera::~RealSenseCamera() {
	printf("Shutting down [Realsense] %s...\n", getCameraName().c_str()); 

	try {
		m_Pipe.stop();
	}
	catch (...) {
		std::cout << "An exception occured while shutting down [Realsense] Camera " << getCameraName();
	}
}

const void *RealSenseCamera::getDepth()
{
	rs2::frameset data = m_Pipe.wait_for_frames(); // Wait for next set of frames from the camera
	rs2::depth_frame depth = data.get_depth_frame();
	m_PixelSize = depth.get_data_size();

	return depth.get_data();
}

size_t RealSenseCamera::getDepthSize()
{
	if (m_PixelSize == 0)
	{
		rs2::frameset data = m_Pipe.wait_for_frames();
		rs2::depth_frame depth = data.get_depth_frame();
		m_PixelSize = depth.get_data_size();
	}

	return m_PixelSize;
}

std::string RealSenseCamera::startRecording(std::string sessionName, unsigned int numFrames)
{
	std::string filename;
	m_Pipe.stop();
	m_Config.enable_record_to_file(sessionName);
	m_Pipe.start(m_Config);

	return filename;
}

void RealSenseCamera::stopRecording()
{
	m_Pipe.stop();
	m_Config.enable_device(mp_Device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	m_Pipe.start(m_Config);
}

void RealSenseCamera::OnUpdate()
{
	m_PointCloud->OnUpdate();
}

inline void RealSenseCamera::OnRender()
{
	m_PointCloud->OnRender();
}

inline void RealSenseCamera::OnImGuiRender()
{
	m_PointCloud->OnImGuiRender();
}

// Utils
void RealSenseCamera::printDeviceInfo() const {
	printf("---\nDevice: %s\n",		mp_Device->get_info(RS2_CAMERA_INFO_NAME));
	printf("Produc Line: %s\n",		mp_Device->get_info(RS2_CAMERA_INFO_PRODUCT_LINE));
	printf("Serial Number: %s\n",	mp_Device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	printf("Physical Port: %s\n\n", mp_Device->get_info(RS2_CAMERA_INFO_PHYSICAL_PORT));
}