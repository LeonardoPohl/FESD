#include "DepthCamera.h"

using namespace rs2;

void RealSenseCamera::getAvailableDevices(context ctx, device_list* rs_devices) {
	*rs_devices = ctx.query_devices();
}

RealSenseCamera::RealSenseCamera(context* ctx) {
	this->_ctx = ctx;
	this->_pipe = pipeline(*ctx);
	this->_pipe.start();
	this->_window_name = this->_pipe.get_active_profile().get_stream(RS2_STREAM_DEPTH).unique_id();
}

RealSenseCamera::RealSenseCamera(context* ctx, device* device) {
	this->_ctx = ctx;
	this->_device = device;
	this->_pipe = pipeline(*ctx);

	this->_cfg->enable_device(device->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	this->_pipe.start(this->_cfg);
}

RealSenseCamera::~RealSenseCamera() {
	this->_pipe.stop();
}

cv::Mat RealSenseCamera::getFrame() {
	frameset data = this->_pipe.wait_for_frames(); // Wait for next set of frames from the camera
	frame depth = data.get_depth_frame().apply_filter(this->_color_map);

	// Query frame size (width and height)
	const int w = depth.as<video_frame>().get_width();
	const int h = depth.as<video_frame>().get_height();

	// Create OpenCV matrix of size (w,h) from the colorized depth data
	return cv::Mat(cv::Size(w, h), CV_8UC3, (void*)depth.get_data());
}

void RealSenseCamera::showFrame() {
	frameset data = this->_pipe.wait_for_frames(); // Wait for next set of frames from the camera
	frame depth = data.get_depth_frame().apply_filter(this->_color_map);

	// Query frame size (width and height)
	const int w = depth.as<video_frame>().get_width();
	const int h = depth.as<video_frame>().get_height();

	// Create OpenCV matrix of size (w,h) from the colorized depth data
	cv::Mat image(cv::Size(w, h), CV_8UC3, (void*)depth.get_data(), cv::Mat::AUTO_STEP);
}