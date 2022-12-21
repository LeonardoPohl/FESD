#pragma once
#include "DepthCamera.h"
#include <OpenNI.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "GLCore/Renderer.h"
#include <obj/Logger.h>

class OrbbecCamera : public DepthCamera {
public:
	OrbbecCamera(openni::DeviceInfo deviceInfo, int camera_id, Logger::Logger* logger);
	~OrbbecCamera() override;

	const void * getDepth() override;

	static std::string getType() { return "Orbbec"; }

	inline std::string getWindowName() const override {
		return "Display: " + this->getCameraName();
	}

	inline std::string getCameraName() const override {
		return this->getType() + " Camera " + std::to_string(this->m_CameraId);
	}

	void printDeviceInfo() const;

	static void getAvailableDevices(openni::Array<openni::DeviceInfo>* available_devices);
	static std::vector<OrbbecCamera *> initialiseAllDevices(Camera *cam, Renderer *renderer, int *starting_id, Logger::Logger* logger);

	inline unsigned int getDepthStreamWidth() const override { return m_DepthWidth; }
	inline unsigned int getDepthStreamHeight() const override { return m_DepthHeight; }

	std::string startRecording(std::string sessionName) override;
	void showCameraInfo() override;
	void saveFrame() override;
	void stopRecording() override;

	void OnUpdate() override;
	void OnRender() override;
	void OnImGuiRender() override;

	void makePointCloud(Camera *cam, Renderer *renderer);

	//https://towardsdatascience.com/inverse-projection-transformation-c866ccedef1c
	inline float getIntrinsics(INTRINSICS intrin) const override
	{
		auto fx = getDepthStreamWidth()  / (2.f * tan(m_hfov / 2.f));
		auto fy = getDepthStreamHeight() / (2.f * tan(m_vfov / 2.f));
		auto cx = getDepthStreamWidth()  / 2;
		auto cy = getDepthStreamHeight() / 2;

		switch (intrin)
		{
			using enum INTRINSICS;
			case FX:
				return fx;
			case FY:
				return fy;
			case CX:
				return (float)cx;
			case CY:
				return (float)cy;
			default:
				break;
		}
		return (float)INFINITE;
	}

	inline glm::mat3 getIntrinsics() const override
	{
		return { getIntrinsics(INTRINSICS::FX),							 0.0f, getIntrinsics(INTRINSICS::CX), 
										  0.0f,	getIntrinsics(INTRINSICS::FY), getIntrinsics(INTRINSICS::CY), 
										  0.0f,							 0.0f,							1.0f };
	}
private:
	void errorHandling(std::string error_string = "");

	openni::DeviceInfo m_DeviceInfo;
	openni::Device m_Device;
	openni::VideoStream m_DepthStream;
	openni::VideoFrameRef m_DepthFrameRef;
	openni::VideoMode m_VideoMode;
	openni::Status m_RC;

	openni::Recorder m_Recorder;
	Logger::Logger* mp_Logger;

	const float m_hfov{ glm::radians(60.0f) };
	const float m_vfov{ glm::radians(49.5f) };
	const float m_dfov{ glm::radians(73.0f) };

	unsigned int m_DepthWidth;
	unsigned int m_DepthHeight;
	std::unique_ptr<GLObject::PointCloud> m_pointcloud;
};
