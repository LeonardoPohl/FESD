#pragma once
#include <OpenNI.h>				// Include OpenNI
#include <opencv2/core.hpp>		// Include OpenCV
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <stdexcept>

class DepthCamera {
public:
	cv::Mat getFrame();
};

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(const openni::DeviceInfo *deviceInfo);
	~OrbbecCamera();

	cv::Mat getFrame();
	void printDeviceInfoOpenni();

	static void getAvailableDevices(openni::Array<openni::DeviceInfo>* available_devices);

private:
	const openni::DeviceInfo* _device_info;
	openni::Device* _device;
	openni::VideoStream* _depth_stream;
	openni::VideoFrameRef _frame_ref;
	openni::Status rc;

};

class RealSenseCamera : public DepthCamera {
public:
	RealSenseCamera(rs2::context* ctx, rs2::device* device);
	~RealSenseCamera();

	cv::Mat getFrame();
	void showFrame();

	static rs2::device_list getAvailableDevices(rs2::context ctx);

private:
	rs2::pipeline _pipe;
	rs2::context* _ctx		{};
	rs2::device* _device	{};
	rs2::config _cfg		{};

	// Declare depth colorizer for pretty visualization of depth data
	rs2::colorizer _color_map{};

	cv::String _window_name	{};
};