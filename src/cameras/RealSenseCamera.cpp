#include "RealsenseCamera.h"
#include <iostream>
#include <exception>
#include "obj/PointCloud.h"
using namespace rs2;

// TODO: Add depth tuning

device_list RealSenseCamera::getAvailableDevices(context ctx) {
	return ctx.query_devices();
}

std::vector<RealSenseCamera*> RealSenseCamera::initialiseAllDevices(Camera* cam, int *starting_id) {
	rs2::context ctx;

	std::vector<RealSenseCamera*> depthCameras;

	for (auto&& dev : RealSenseCamera::getAvailableDevices(ctx))
	{
		depthCameras.push_back(new RealSenseCamera(&ctx, &dev, cam, (*starting_id)++));
		std::cout << "Initialised " << depthCameras.back()->getCameraName() << std::endl;
	}

	return depthCameras;
}

RealSenseCamera::RealSenseCamera(context* ctx, device* device, Camera *cam, int camera_id) :
	_pipe(pipeline(*ctx)), 
	_ctx(ctx), 
	_device(device)
	 {
	this->camera_id = camera_id;

	this->printDeviceInfo();
	this->_device->query_sensors();
	this->_cfg.enable_device(this->_device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	
	auto profile = this->_pipe.start(this->_cfg);

	frameset data = this->_pipe.wait_for_frames(); // Wait for next set of frames from the camera
	frame depth = data.get_depth_frame();

	// Get Intrinsics
	auto depth_profile = depth.get_profile().as<rs2::video_stream_profile>();
	this->intrinsics   = depth_profile.get_intrinsics();
	
	// Query frame size (width and height)
	this->depth_width  = intrinsics.width;
	this->depth_height = intrinsics.height;

	m_pointcloud = std::make_unique<GLObject::PointCloud>(this, cam);
}

RealSenseCamera::~RealSenseCamera() {
	printf("Shutting down [Realsense] %s...\n", this->getCameraName().c_str()); 

	try {
		this->_pipe.stop();
	}
	catch (...) {
		std::cout << "An exception occured while shutting down [Realsense] Camera " << this->getCameraName();
	}
}

const void *RealSenseCamera::getDepth()
{
	frameset data = this->_pipe.wait_for_frames(); // Wait for next set of frames from the camera
	depth_frame depth = data.get_depth_frame();
	pixel_size = depth.get_data_size();

	return depth.get_data();
}

size_t RealSenseCamera::getDepthSize()
{
	if (pixel_size != 0)
	{
		return pixel_size;
	}
	frameset data = this->_pipe.wait_for_frames(); // Wait for next set of frames from the camera
	depth_frame depth = data.get_depth_frame();
	pixel_size = depth.get_data_size();

	return depth.get_data_size();
}

void RealSenseCamera::startRecording(std::string sessionName, long long startOn, unsigned int numFrames)
{

}

void RealSenseCamera::stopRecording()
{

}

void RealSenseCamera::OnUpdate()
{
	m_pointcloud->OnUpdate();
}

inline void RealSenseCamera::OnRender()
{
	m_pointcloud->OnRender();
}

inline void RealSenseCamera::OnImGuiRender()
{

	m_pointcloud->OnImGuiRender();
}

// Utils
void RealSenseCamera::printDeviceInfo() const {
	printf("---\nDevice: %s\n", this->_device->get_info(RS2_CAMERA_INFO_NAME));
	printf("Produc Line: %s\n", this->_device->get_info(RS2_CAMERA_INFO_PRODUCT_LINE));
	printf("Serial Number: %s\n", this->_device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	printf("Physical Port: %s\n\n", this->_device->get_info(RS2_CAMERA_INFO_PHYSICAL_PORT));
}