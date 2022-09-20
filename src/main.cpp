/// Main.cpp

#include <ImguiBootstrap.h>
#include <OpenNI.h>
#include <App.h>

int main() {
    auto glsl_version = imgui::glfw_init();
    if (!glsl_version) {
        return 1;
    }

    //# initialize openni sdk
    //#######################

    if (openni::OpenNI::initialize() != openni::STATUS_OK)
    {
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());
        return 1;
    }

    App(*glsl_version);

    //# Shutdown
    //##########

    openni::OpenNI::shutdown();

    return 0;
}

