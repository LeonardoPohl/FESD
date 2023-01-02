#pragma once
#include <vector>
#include <chrono>
#include <json/json.h>

#include "DepthCamera.h"
#include "GLCore/Camera.h"
#include "GLCore/Renderer.h"
#include "obj/Logger.h"

class CameraHandler
{
public:
	CameraHandler(Camera *cam, Renderer *r, Logger::Logger *logger);
	~CameraHandler();

	void initAllCameras();
	void OnRender();
	void OnImGuiRender();
private:
	void showSessionSettings();
	void showRecordingStats();
	void showRecordings();
	void startRecording();
	void stopRecording();
	void findRecordings();
	void calculateSkeleton();

	void clearCameras();
	void updateSessionName();
	std::string getFileSafeSessionName();

	enum State {
		Streaming,
		Recording,
		Playback,
		HPE
	};

	State m_State;
	std::string m_SessionName{ "Session" };

	Camera *mp_Camera;
	Renderer *mp_Renderer;

	Logger::Logger* mp_Logger;

	std::vector<DepthCamera *> m_DepthCameras;
	std::vector<Json::Value> m_Recordings;

	std::chrono::time_point<std::chrono::system_clock> m_RecordingStart;
	std::chrono::time_point<std::chrono::system_clock> m_RecordingEnd;
	std::chrono::duration<double> m_RecordedSeconds;

	bool m_StreamWhileRecording{ true };
	bool m_LimitFrames{ false };
	bool m_LimitTime{ true };
	bool m_PlaybackPaused{ false };

	int m_FrameLimit{ 100 };

	/// <summary>
	/// Time Limit in Seconds
	/// </summary>
	int m_TimeLimit{ 100 };

	int m_RecordedFrames{ 0 };
};

