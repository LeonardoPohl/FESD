#pragma once
#include <vector>
#include <chrono>
#include <string>
#include <memory>

#include <GLCore/Camera.h>
#include <GLCore/Renderer.h>

#include "DepthCamera.h"
#include "obj/Logger.h"
#include "obj/SkeletonDetector.h"

class CameraHandler
{
public:
	CameraHandler(Camera *cam, Renderer *r, Logger::Logger *logger);
	~CameraHandler();

	void initAllCameras();
	void OnUpdate();
	void OnImGuiRender();
private:
	void showSessionSettings();
	void showRecordingStats();
	void showRecordings();
	void startRecording();
	void stopRecording();
	void findRecordings();

	void clearCameras();
	void updateSessionName();
	std::string getFileSafeSessionName();

	enum State {
		Streaming,
		Recording,
		Playback
	};

	State m_State;

	Camera *mp_Camera;
	Renderer *mp_Renderer;

	Logger::Logger* mp_Logger;

	// Streaming
	bool m_CamerasExist{ false };
	bool m_ShowColorFrames{ false };
	std::unique_ptr<GLObject::PointCloud> m_PointCloud;
	std::vector<DepthCamera *> m_DepthCameras;

	// Recording
	std::string m_SessionName{ };
	bool m_StreamWhileRecording{ true };
	bool m_LimitFrames{ false };
	bool m_LimitTime{ true };
	int m_FrameLimit{ 100 };
	int m_TimeLimitInS{ 100 };
	std::chrono::time_point<std::chrono::system_clock> m_RecordingStart;
	std::chrono::time_point<std::chrono::system_clock> m_RecordingEnd;
	std::chrono::duration<double> m_RecordedSeconds;
	int m_RecordedFrames{ 0 };

	// Playback
	std::vector<Json::Value> m_Recordings;
	bool m_PlaybackPaused{ false };

	// Skeleton Detection
	bool m_DoSkeletonDetection{ false };
	float m_ScoreThreshold{ 0.0f };
	bool m_ShowUncertainty{ false };
	std::unique_ptr<SkeletonDetector> m_SkeletonDetector;
};

