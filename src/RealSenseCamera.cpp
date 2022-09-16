#include "DepthCamera.h"
#include <opencv2/highgui.hpp>
#include <iostream>

using namespace rs2;

device_list RealSenseCamera::getAvailableDevices(context ctx) {
	return ctx.query_devices();
}

std::vector<RealSenseCamera*> RealSenseCamera::initialiseAllDevices() {
	rs2::context ctx;

	std::vector<RealSenseCamera*> depthCameras;

	int camera_id = 0;
	for (auto&& dev : RealSenseCamera::getAvailableDevices(ctx))
	{
		depthCameras.push_back(new RealSenseCamera(&ctx, &dev, camera_id));
		std::cout << "Initialised " << depthCameras.back()->getCameraName() << std::endl;
		camera_id++;
	}

	return depthCameras;
}

RealSenseCamera::RealSenseCamera(context* ctx, device* device, int camera_id) :
	_pipe(pipeline(*ctx)), 
	_ctx(ctx), 
	_device(device),
	camera_id(camera_id) {
	this->printDeviceInfo();
	
	this->_cfg.enable_device(this->_device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	this->_pipe.start(this->_cfg);
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

cv::Mat RealSenseCamera::getFrame() {
	frameset data = this->_pipe.wait_for_frames(); // Wait for next set of frames from the camera
	frame depth = data.get_depth_frame();//.apply_filter(this->_color_map);

	// Query frame size (width and height)
	const int w = depth.as<video_frame>().get_width();
	const int h = depth.as<video_frame>().get_height();

	// Create OpenCV matrix of size (w,h) from the colorized depth data
	return cv::Mat(cv::Size(w, h), CV_16UC1, (void*)depth.get_data(), cv::Mat::AUTO_STEP);
}

// Utils
void RealSenseCamera::printDeviceInfo() const {
	printf("---\nDevice: %s\n", this->_device->get_info(RS2_CAMERA_INFO_NAME));
	printf("Produc Line: %s\n", this->_device->get_info(RS2_CAMERA_INFO_PRODUCT_LINE));
	printf("Serial Number: %s\n", this->_device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	printf("Physical Port: %s\n\n", this->_device->get_info(RS2_CAMERA_INFO_PHYSICAL_PORT));
}