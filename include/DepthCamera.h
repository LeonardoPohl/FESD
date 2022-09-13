#pragma once
#include <OpenNI.h>				// Include OpenNI
#include <opencv2/core.hpp>		// Include OpenCV
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <stdexcept>

class DepthCamera {
public:
	virtual ~DepthCamera() { };
	virtual cv::Mat getFrame() = 0;
};

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(const openni::DeviceInfo *deviceInfo, std::string window_name);
	~OrbbecCamera();

	cv::Mat getFrame();
	void printDeviceInfo();

	static void getAvailableDevices(openni::Array<openni::DeviceInfo>* available_devices);

private:
	const openni::DeviceInfo* _device_info;
	openni::Device _device;
	openni::VideoStream _depth_stream;
	openni::VideoFrameRef _frame_ref;
	openni::Status rc;

	std::string _window_name{};
};

class RealSenseCamera : public DepthCamera {
public:
	RealSenseCamera(rs2::context* ctx, rs2::device* device, std::string window_name);
	~RealSenseCamera();

	cv::Mat getFrame();
	void printDeviceInfo();

	static rs2::device_list getAvailableDevices(rs2::context ctx);

private:
	rs2::pipeline _pipe;
	rs2::context* _ctx		{};
	rs2::device* _device	{};
	rs2::config _cfg		{};

	// Declare depth colorizer for pretty visualization of depth data
	rs2::colorizer _color_map{};

	std::string _window_name	{};
};