#pragma once
#include <vector>
#include "DepthCamera.h"
#include "GLCore/Camera.h"
#include "GLCore/Renderer.h"

#include <filesystem>
#include <json/json.h>

namespace fs = std::filesystem;

class CameraHandler
{
public:
	CameraHandler(Camera *cam, Renderer *r);
	~CameraHandler();

	void findAllCameras();
	void initAllCameras();
	void showCameras();
	void OnRender();
	void OnImGuiRender();
private:
	enum State {
		Streaming,
		Recording
	};

	State m_State;
	char m_SessionName[120]{ "Session" };

	Camera *mp_Camera;
	Renderer *mp_Renderer;

	std::vector<DepthCamera *> m_DepthCameras;
	std::vector<Json::Value> m_Recordings;
	
	bool m_DoingPlayback{ false };

	fs::path m_RecordingDirectory{ "F:\\Recordings" };
};

