#pragma once
#include <vector>
#include "DepthCamera.h"
#include "GLCore/Camera.h"

namespace Params
{
	class GlobalParameters;
	class SphereDetectionParameters;
	class NormalParameters;
}
class CameraHandler
{
public:
	CameraHandler(Camera *cam);
	~CameraHandler();

	void findAllCameras();
	void initAllCameras();
	void showCameras();
	void OnUpdate();
	void OnRender();
	void OnImGuiRender();
private:
	Camera *cam;

	std::vector<DepthCamera *> depthCameras;

	std::unique_ptr<Params::GlobalParameters> global_params;
	std::unique_ptr<Params::NormalParameters> normal_params;
};

