#pragma once

#include "DepthCamera.h"
#include <librealsense2/rs.hpp>
#include <memory>

class RealSenseCamera : public DepthCamera {
public:
	RealSenseCamera(rs2::context* ctx, rs2::device* device, Camera *cam, int camera_id);
	~RealSenseCamera() override;

	const void * getDepth() override;
	size_t getDepthSize() override;

	std::string getName() const override { return "Realsense"; }

	void printDeviceInfo() const;

	static rs2::device_list getAvailableDevices(rs2::context ctx);
	static std::vector<RealSenseCamera*> initialiseAllDevices(Camera* cam, int *starting_id);

	inline unsigned int getDepthStreamWidth() const override
	{
		return depth_width;
	}

	inline unsigned int getDepthStreamHeight() const override
	{
		return depth_height;
	}

	inline uint16_t getDepthStreamMaxDepth() const override
	{
		return max_depth;
	}

	void startRecording(std::string sessionName, long long startOn, unsigned int numFrames = 0) override;
	void stopRecording() override;

	void OnUpdate() override;
	void OnRender() override;
	void OnImGuiRender() override;
private:
	rs2::pipeline _pipe;
	rs2::context* _ctx{};
	rs2::device* _device{};
	rs2::config _cfg{};
	rs2_intrinsics intrinsics;

	size_t pixel_size{ 0 };

	// Declare depth colorizer for pretty visualization of depth data
	rs2::colorizer _color_map{};
	unsigned int max_depth { 32766 };

	unsigned int depth_width;
	unsigned int depth_height;
	std::unique_ptr<GLObject::PointCloud> m_pointcloud;
};
