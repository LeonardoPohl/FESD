#pragma once

#include "DepthCamera.h"

class RealSenseCamera : public DepthCamera {
public:
	RealSenseCamera(rs2::context* ctx, rs2::device* device, int camera_id);
	~RealSenseCamera() override;

	cv::Mat getDepthFrame() override;
	cv::Mat getColorFrame() override;
	bool hasColorStream() { return true; };
	std::string getName() const override { return "Realsense"; }
	cv::Vec3f pixelToPoint(int x, int y, ushort depth) const override;

	void printDeviceInfo() const;

	static rs2::device_list getAvailableDevices(rs2::context ctx);
	static std::vector<RealSenseCamera*> initialiseAllDevices();
private:
	rs2::pipeline _pipe;
	rs2::context* _ctx{};
	rs2::device* _device{};
	rs2::config _cfg{};
	rs2_intrinsics intrinsics;

	// Declare depth colorizer for pretty visualization of depth data
	rs2::colorizer _color_map{};
	int camera_id;
};
