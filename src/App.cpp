#include "App.h"

App::App(std::string_view const& glsl_version) {

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
    cv::Mat depth_frame, edge_frame, color_frame, frame, normal_frame, floor_points;
    std::vector<int>::iterator new_end;
    bool reset_walking_frames = false;

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

            try {
                depth_frame = cam->getDepthFrame();

                if (global_params.walking_average) {
                    cam->walkingFrames.enqueue(depth_frame);
                    depth_frame = cam->walkingFrames.getValue();
                }

                if (cam->show_color_stream) {
                    color_frame = cam->getColorFrame();
                    if (color_frame.rows > 0 && color_frame.cols > 0) {
                        cv::imshow(cam->getWindowName() + " - Color Stream", color_frame);
                        cv::resizeWindow(cam->getWindowName() + " - Color Stream", color_frame.size[1], color_frame.size[0]);
                    }
                }

                //# Display image
                //###############

                if (global_params.display_edges) {
                    edge_frame = DepthCamera::detectEdges(depth_frame, sphere_params);
                    if (global_params.walking_average) {
                        cam->walkingEdges.enqueue(edge_frame);
                        edge_frame = cam->walkingEdges.getValue();
                    }

                    if (edge_frame.rows > 0 && edge_frame.cols > 0) {
                        cv::imshow(cam->getWindowName() + " - Edges", edge_frame);
                        cv::resizeWindow(cam->getWindowName() + " - Edges", edge_frame.size[1], edge_frame.size[0]);
                    }
                }

                //# Sphere Detection
                //##################

                if (cam->detect_circles) {
                    cam->displaySphereTable(depth_frame, edge_frame, sphere_params, global_params.display_edges);
                }

                if (cam->selectedFloorPoint.x != -1) {
                    floor_points = cam->calculateSelectedFloor(depth_frame, normal_params);

                    if (floor_points.rows > 0 && floor_points.cols > 0) {
                        cv::imshow(cam->getWindowName() + " Floor", floor_points);
                        cv::resizeWindow(cam->getWindowName() + " Floor", floor_points.size[1], floor_points.size[0]);
                    }
                }
                if (depth_frame.rows > 0 && depth_frame.cols > 0) {
                    cv::imshow(cam->getWindowName(), depth_frame);
                    cv::resizeWindow(cam->getWindowName(), depth_frame.size[1], depth_frame.size[0]);
                }
            }
            catch (cv::Exception e) {
                std::cout << " | " << e.msg;
                if (cam->detect_circles) {
                    cam->detect_circles = false;
                }
                else {
                    cam->is_enabled = false;
                }
            }
            catch (std::exception e) {
                std::cout << " | " << e.what();
                if (cam->detect_circles) {
                    cam->detect_circles = false;
                }
                else {
                    cam->is_enabled = false;
                }
            }
            ImGui::End();
        }
    }
}