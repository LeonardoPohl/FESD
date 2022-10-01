/// Main.cpp

#include <ImguiBootstrap.h>
#include <OpenNI.h>
#include <App.h>

int main() {

    //# initialize openni sdk
    //#######################

    if (openni::OpenNI::initialize() != openni::STATUS_OK)
    {
        printf("Initialization of OpenNi failed\n%s\n", openni::OpenNI::getExtendedError());
        return 1;
    }

    App();

    //# Shutdown
    //##########

    openni::OpenNI::shutdown();

    return 0;
}

