#pragma once
#include <vector>
#include "DepthCamera.h"
#include "GLCore/Camera.h"

class CameraHandler
{
public:
	CameraHandler(Camera *cam);
	~CameraHandler();

	void findAllCameras();
	void initAllCameras();
	void showCameras();
	void OnRender();
	void OnImGuiRender();
private:
	Camera *cam;

	std::vector<DepthCamera *> depthCameras;
};

