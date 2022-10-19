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
	this->_pipe.start(this->_cfg);

	frameset data = this->_pipe.wait_for_frames(); // Wait for next set of frames from the camera
	frame depth = data.get_depth_frame();

	// Query frame size (width and height)
	this->depth_width = depth.as<video_frame>().get_width();
	this->depth_height = depth.as<video_frame>().get_height();

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

const uint16_t *RealSenseCamera::getDepth()
{
	frameset data = this->_pipe.wait_for_frames(); // Wait for next set of frames from the camera
	frame depth = data.get_depth_frame();

	return (uint16_t *)depth.get_data();
}

void RealSenseCamera::startRecording(std::string sessionName, long long startOn, unsigned int numFrames)
{

}

void RealSenseCamera::stopRecording()
{

}

void RealSenseCamera::OnUpdate()
{

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