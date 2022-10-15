#pragma once
#include "DepthCamera.h"
#include <OpenNI.h>
#include <vector>
#include <memory>

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(const openni::DeviceInfo* deviceInfo, int camera_id);
	~OrbbecCamera() override;

	const uint16_t * getDepth() override;
	std::string getName() const override { return "Orbbec"; }

	void printDeviceInfo() const;

	static void getAvailableDevices(openni::Array<openni::DeviceInfo>* available_devices);
	static std::vector<OrbbecCamera*> initialiseAllDevices(int *starting_id);

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

	void OnPointCloudRender() const override;
	void OnPointCloudOnImGuiRender() const override;
private:
	const openni::DeviceInfo* _device_info;
	openni::Device _device;
	openni::VideoStream _depth_stream;
	openni::VideoFrameRef _frame_ref;
	openni::VideoMode _video_mode;
	openni::Status rc;
	unsigned int max_depth;

	unsigned int depth_width;
	unsigned int depth_height;
	std::unique_ptr<GLObject::PointCloud> m_pointcloud;
};
