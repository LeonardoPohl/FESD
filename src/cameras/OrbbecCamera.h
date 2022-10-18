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

	inline unsigned int getDepthStreamWidth() const override { return depth_width; }
	inline unsigned int getDepthStreamHeight() const override { return depth_height; }
	inline uint16_t getDepthStreamMaxDepth() const override { return max_depth; }

	void startRecording(std::string sessionName, long long startOn, unsigned int numFrames = 0) override;
	void stopRecording() override;

	void OnUpdate() override;
	void OnRender() override;
	void OnImGuiRender() override;
	inline void setNumFrames(int numFrames)
	{
		this->frames_left = numFrames;
		this->num_frames = numFrames;
	}
	inline bool decFramesLeft()
	{
		return this->frames_left-- <= 0 ;
	}
private:
	const openni::DeviceInfo* _device_info;
	openni::Device _device;
	openni::VideoStream _depth_stream;
	openni::VideoFrameRef _frame_ref;
	openni::VideoMode _video_mode;
	openni::Status rc;
	unsigned int max_depth;

	int frames_left{ 0 };
	int num_frames{ 0 };
	int delay{ 0 };
	bool limit_frames = true;

	unsigned int depth_width;
	unsigned int depth_height;
	std::unique_ptr<GLObject::PointCloud> m_pointcloud;
};
