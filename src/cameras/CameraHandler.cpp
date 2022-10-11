#include "CameraHandler.h"

#include <opencv2/highgui.hpp>
#include <imgui.h>

#include <parameters/Parameters.h>
#include "RealsenseCamera.h"
#include "OrbbecCamera.h"

#include <OpenNI.h>
#include <iostream>

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

void CameraHandler::findAllCameras()
{
    depthCameras.clear();

    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices();
    auto rs_cameras = RealSenseCamera::initialiseAllDevices();

    std::cout << "[INFO] Queried all devices" << std::endl;

    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());
}

void CameraHandler::initAllCameras()
{
    depthCameras.clear();

    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices();
    auto rs_cameras = RealSenseCamera::initialiseAllDevices();

    std::cout << "[INFO] Queried all devices" << std::endl;

    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());
}

void CameraHandler::showCameras()
{
    for (auto cam : depthCameras)
    {
        ImGui::Checkbox(cam->getCameraName().c_str(), &cam->is_enabled);
    }
}