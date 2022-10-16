#pragma once

#include "DepthCamera.h"
#include <librealsense2/rs.hpp>
#include <memory>

class RealSenseCamera : public DepthCamera {
public:
	RealSenseCamera(rs2::context* ctx, rs2::device* device, int camera_id);
	~RealSenseCamera() override;

	const uint16_t * getDepth() override;
	std::string getName() const override { return "Realsense"; }

	void printDeviceInfo() const;

	static rs2::device_list getAvailableDevices(rs2::context ctx);
	static std::vector<RealSenseCamera*> initialiseAllDevices(int *starting_id);

	inline unsigned int getDepthStreamWidth() const override
	{
		return depth_width;
	}

	inline unsigned int getDepthStreamHeight() const override
	{
		return depth_height;
	}

	inline unsigned int getDepthStreamMaxDepth() const override
	{
		return max_depth;
	}

	inline void OnPointCloudRender() const override;
	inline void OnPointCloudOnImGuiRender() const override;
private:
	rs2::pipeline _pipe;
	rs2::context* _ctx{};
	rs2::device* _device{};
	rs2::config _cfg{};
	rs2_intrinsics intrinsics;

	// Declare depth colorizer for pretty visualization of depth data
	rs2::colorizer _color_map{};
	unsigned int max_depth;

	unsigned int depth_width;
	unsigned int depth_height;
	std::unique_ptr<GLObject::PointCloud> m_pointcloud;
};
