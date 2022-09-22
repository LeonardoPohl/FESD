#include "DepthCamera.h"
#include <opencv2/highgui.hpp>
#include <iostream>
#include <exception>

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
	this->_device->query_sensors();
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
	frame depth = data.get_depth_frame();

	// Query frame size (width and height)
	const int w = depth.as<video_frame>().get_width();
	const int h = depth.as<video_frame>().get_height();

	// Create OpenCV matrix of size (w,h) from the colorized depth data
	return cv::Mat(cv::Size(w, h), CV_16UC1, (void*)depth.get_data(), cv::Mat::AUTO_STEP) * 10;
}

cv::Point3f RealSenseCamera::pixelToPoint(int x, int y, ushort depth) const
{
	float pixel[3] = { x, y, depth };
	rs2_intrinsics* intrinsics;
	rs2_stream_profile* profile;
	//rs2_open(, profile, this->roc);
	//rs2_get_video_stream_intrinsics(RS2_STREAM_DEPTH, intrinsics);
	//rs2_deproject_pixel_to_point(pt, intrinsics)
	cv::Point3f pt;
	//CoordinateConverter::convertDepthToWorld(this->_depth_stream, x, y, ((DepthPixel*)this->_frame_ref.getData())[x * this->_frame_ref.getWidth() + y], &pt.x, &pt.y, &pt.z);
	return pt;
}

// Utils
void RealSenseCamera::printDeviceInfo() const {
	printf("---\nDevice: %s\n", this->_device->get_info(RS2_CAMERA_INFO_NAME));
	printf("Produc Line: %s\n", this->_device->get_info(RS2_CAMERA_INFO_PRODUCT_LINE));
	printf("Serial Number: %s\n", this->_device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	printf("Physical Port: %s\n\n", this->_device->get_info(RS2_CAMERA_INFO_PHYSICAL_PORT));
}