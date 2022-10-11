#pragma once

#include "DepthCamera.h"
#include <librealsense2/rs.hpp>

class RealSenseCamera : public DepthCamera {
public:
	RealSenseCamera(rs2::context* ctx, rs2::device* device, int camera_id);
	~RealSenseCamera() override;

	const void* getDepth() override;
	std::string getName() const override { return "Realsense"; }

	void printDeviceInfo() const;

	static rs2::device_list getAvailableDevices(rs2::context ctx);
	static std::vector<RealSenseCamera*> initialiseAllDevices(int *starting_id);
private:
	rs2::pipeline _pipe;
	rs2::context* _ctx{};
	rs2::device* _device{};
	rs2::config _cfg{};
	rs2_intrinsics intrinsics;

	// Declare depth colorizer for pretty visualization of depth data
	rs2::colorizer _color_map{};
};
