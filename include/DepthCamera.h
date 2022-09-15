#pragma once
#include <OpenNI.h>				// Include OpenNI
#include <opencv2/core.hpp>		// Include OpenCV
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <stdexcept>
#include <Circle.h>

class DepthCamera {
public:
	virtual ~DepthCamera() = default;
	virtual cv::Mat getFrame() = 0;

	virtual std::vector<Circle*> detectSpheres();
};

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(const openni::DeviceInfo *deviceInfo, const char* window_name);
	~OrbbecCamera() override;

	cv::Mat getFrame() override;
	void printDeviceInfo() const;

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
	RealSenseCamera(rs2::context* ctx, rs2::device* device, const char* window_name);
	~RealSenseCamera() override;

	cv::Mat getFrame() override;
	void printDeviceInfo() const;

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