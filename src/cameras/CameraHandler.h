#pragma once
#include <vector>
#include "DepthCamera.h"
#include "GLCore/Camera.h"
#include "GLCore/Renderer.h"
#include <filesystem>

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

	std::filesystem::path m_RecordingDirectory{ "F:\\Recordings" };
};

