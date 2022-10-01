#include "App.h"
#include <OrbbecCamera.h>
#include <RealsenseCamera.h>

App::App() {

    auto glsl_version = imgui::glfw_init();
    if (glsl_version.empty()) {
        return;
    }

    //# initialize imgui
    //##################

    window = imgui::create_window(glsl_version);
    if (window == nullptr) {
        return;
    }

    auto clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    io = &ImGui::GetIO();
    io->Fonts->AddFontDefault();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(2);

    //# Camera Initialisation
    //#######################

    initAllCameras();

    //# Main Loop
    //###########

    auto count = 0;

    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        update();

        imgui::render(window, clear_color);
    }

    std::cout << std::endl;

    for (DepthCamera* cam : depthCameras) {
        delete cam;
    }
}

void App::initAllCameras() {
    auto rs_cameras = RealSenseCamera::initialiseAllDevices();
    auto orbbec_cameras = OrbbecCamera::initialiseAllDevices();

    depthCameras.insert(depthCameras.end(), rs_cameras.begin(), rs_cameras.end());
    depthCameras.insert(depthCameras.end(), orbbec_cameras.begin(), orbbec_cameras.end());

    for (DepthCamera* cam : depthCameras) {
        cv::namedWindow(cam->getWindowName());
        cv::setMouseCallback(cam->getWindowName(), onMouse, (void*)(&cam->selectedFloorPoint));
    }
}

void App::update() {
    bool reset_walking_frames = false;
    global_params.displayParameters();
    normal_params.displayParameters();
    sphere_params.displayParameters();
    if (reset_walking_frames) {
        for (DepthCamera* cam : depthCameras) {
            cam->walkingFrames.reset();
            cam->walkingEdges.reset();
        }
    }

    for (DepthCamera* cam : depthCameras) {

        if (cam->is_enabled) {
            ImGui::Begin(cam->getCameraName().c_str());
            if (global_params.walking_average) {
                if (ImGui::Button("Flush Moving Average")) {
                    cam->walkingFrames.reset();
                    cam->walkingEdges.reset();
                }
                if (ImGui::SliderInt("Moving Average Length", &cam->walkingFrames.length, 1, 100)) {
                    cam->walkingEdges.length = cam->walkingFrames.length;
                }

            }
            ImGui::Checkbox("Detect Spheres", &cam->detect_circles);

            if (cam->hasColorStream()) {
                ImGui::Checkbox("Color Stream", &cam->show_color_stream);
            }
            else if (cam->show_color_stream) {
                cam->show_color_stream = false;
            }
            ImGui::End();

            cam->doUpdate(&global_params, &normal_params, &sphere_params);
        }
    }
}