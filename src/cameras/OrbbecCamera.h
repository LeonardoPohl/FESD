#pragma once
#include <OpenNI.h>
#include <vector>
#include <filesystem>
#include <memory>
#include <glm/glm.hpp>

#include "DepthCamera.h"
#include "GLCore/Renderer.h"
#include "obj/Logger.h"
#include "utilities/helper/ImGuiHelper.h"

class OrbbecCamera : public DepthCamera {
public:
	/// Constructors & Destructors
	OrbbecCamera(openni::DeviceInfo deviceInfo, Camera* cam, Renderer* renderer, int camera_id, Logger::Logger* logger);
	OrbbecCamera(Camera* cam, Renderer* renderer, Logger::Logger* logger, std::filesystem::path recording);
	~OrbbecCamera() override;

	/// Initialise all devices
	static void getAvailableDevices(openni::Array<openni::DeviceInfo>* available_devices);
	static std::vector<OrbbecCamera*> initialiseAllDevices(Camera* cam, Renderer* renderer, int* starting_id, Logger::Logger* logger);

	/// Camera Details
	static std::string getType();
	inline std::string getCameraName() const;
	void showCameraInfo() override;
	void printDeviceInfo() const;
	inline unsigned int getDepthStreamWidth() const override { return m_DepthWidth; }
	inline unsigned int getDepthStreamHeight() const override { return m_DepthHeight; }
	inline float getIntrinsics(INTRINSICS intrin) const override;
	inline glm::mat3 getIntrinsics() const override;

	/// Frame retreival
	const void * getDepth() override;
	cv::Mat getColorFrame() override;

	/// Frame update
	void OnUpdate() override;
	void OnRender() override;
	void OnImGuiRender() override;

	/// Recording
	std::string startRecording(std::string sessionName) override;
	void saveFrame() override;
	void stopRecording() override;
private:
	void errorHandling(std::string error_string = "");

	openni::DeviceInfo m_DeviceInfo;
	openni::Device m_Device;
	openni::VideoStream m_DepthStream;
	openni::VideoFrameRef m_DepthFrameRef;
	openni::VideoMode m_VideoMode;
	openni::Status m_RC;

	openni::Recorder m_Recorder;
	openni::PlaybackControl *mp_PlaybackController;

	cv::VideoCapture m_ColorStream;
	cv::Mat m_ColorFrame{ };
	cv::VideoWriter m_ColorStreamRecorder;
	
	int m_CurrentPlaybackFrame{ 0 };
	bool m_IsRecording{ false };
	bool m_IsPlayback{ false };
	bool m_PlaybackHasRGBStream{ true };

	Logger::Logger* mp_Logger;

	const float m_hfov{ glm::radians(60.0f) };
	const float m_vfov{ glm::radians(49.5f) };
	const float m_dfov{ glm::radians(73.0f) };

	unsigned int m_DepthWidth;
	unsigned int m_DepthHeight;
	std::unique_ptr<GLObject::PointCloud> m_PointCloud;

	int m_CVCameraId{ 0 };
	int m_CVCameraSearchDepth{ 10 };
	bool m_CVCameraFound{ false };
};
