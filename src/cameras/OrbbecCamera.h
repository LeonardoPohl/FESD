#pragma once
#include "DepthCamera.h"
#include <OpenNI.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "GLCore/Renderer.h"

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(const openni::DeviceInfo* deviceInfo, int camera_id);
	~OrbbecCamera() override;

	const void * getDepth() override;
	//inline size_t getDepthSize() override { return sizeof(int16_t); }

	std::string getName() const override { return "Orbbec"; }

	void printDeviceInfo() const;

	static void getAvailableDevices(openni::Array<openni::DeviceInfo>* available_devices);
	static std::vector<OrbbecCamera *> initialiseAllDevices(Camera *cam, Renderer *renderer, int *starting_id);

	inline unsigned int getDepthStreamWidth() const override { return depth_width; }
	inline unsigned int getDepthStreamHeight() const override { return depth_height; }

	std::string startRecording(std::string sessionName, unsigned int numFrames = 0) override;
	void stopRecording() override;

	void OnUpdate() override;
	void OnRender() override;
	void OnImGuiRender() override;

	void makePointCloud(Camera *cam, Renderer *renderer);

	inline void setNumFrames(int numFrames)
	{
		this->frames_left = numFrames;
		this->num_frames = numFrames;
	}
	inline bool decFramesLeft()
	{
		return this->frames_left-- <= 0 ;
	}

	//https://towardsdatascience.com/inverse-projection-transformation-c866ccedef1c
	inline float getIntrinsics(INTRINSICS intrin) const override
	{
		auto fx = getDepthStreamWidth() / (2.f * tan(hfov / 2.f));
		auto fy = getDepthStreamHeight() / (2.f * tan(vfov / 2.f));
		auto cx = getDepthStreamWidth() / 2;
		auto cy = getDepthStreamHeight() / 2;

		switch (intrin)
		{
			using enum INTRINSICS;
			case FX:
				return fx;
			case FY:
				return fy;
			case CX:
				return cx;
			case CY:
				return cy;
			default:
				break;
		}
		return INFINITE;
	}

	inline glm::mat3 getIntrinsics() const override
	{
		return { getIntrinsics(INTRINSICS::FX),							 0.0f, getIntrinsics(INTRINSICS::CX), 
										  0.0f,	getIntrinsics(INTRINSICS::FY), getIntrinsics(INTRINSICS::CY), 
										  0.0f,							 0.0f,							1.0f };
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

	const float hfov{ glm::radians(60.0f) };
	const float vfov{ glm::radians(49.5f) };
	const float dfov{ glm::radians(73.0f) }; // no Idea what that is

	unsigned int depth_width;
	unsigned int depth_height;
	std::unique_ptr<GLObject::PointCloud> m_pointcloud;
};
