#include "CameraHandler.h"

#include <opencv2/highgui.hpp>
#include <imgui.h>

#include <parameters/Parameters.h>
#include "RealsenseCamera.h"
#include "OrbbecCamera.h"

#include <OpenNI.h>

CameraHandler::CameraHandler()
{
    if (openni::OpenNI::initialize() != openni::STATUS_OK)
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());

    global_params = std::make_unique<Params::GlobalParameters>(&depthCameras);
    sphere_params = std::make_unique<Params::SphereDetectionParameters>();
    normal_params = std::make_unique<Params::NormalParameters>();
}

CameraHandler::~CameraHandler()
{
    for (auto cam : depthCameras)
    {
        delete cam;
    }
    openni::OpenNI::shutdown();
}

void CameraHandler::initAllCameras()
{
    depthCameras.clear();

    auto rs_cameras = RealSenseCamera::initialiseAllDevices();
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices();

    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());

    for (auto cam : depthCameras)
    {
        cv::namedWindow(cam->getWindowName());
    }
}

void CameraHandler::showCameras()
{
    for (auto cam : depthCameras)
    {
        ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);
    }
}