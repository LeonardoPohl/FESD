#pragma once

#include "obj/tests/TestClearColor.h"
#include "obj/tests/TestTriangle2D.h"
#include "obj/tests/TestTexture2D.h"
#include "obj/tests/TestPyramid3D.h"
#include "obj/tests/TestPoint.h"

#include "GLCore/Camera.h"

class TestMenuHelper
{
public:
	TestMenuHelper(Camera const *cam)
	{
        testMenu = new GLObject::TestMenu(currentTest, cam);
        currentTest = testMenu;

        testMenu->RegisterTest<GLObject::TestClearColor>("Clear Color");
        testMenu->RegisterTest<GLObject::TestTriangle2D>("2D Plane");
        testMenu->RegisterTest<GLObject::TestTexture2D>("2D Texture");
        testMenu->RegisterTest<GLObject::TestPyramid3D>("3D Pyramid");
        testMenu->RegisterTest<GLObject::TestPoint>("3D Point");
	}

    ~TestMenuHelper()
    {
        delete currentTest;
        if (currentTest != testMenu)
            delete testMenu;
    }

    void update()
    {
        if (currentTest)
        {
            currentTest->OnUpdate();
            currentTest->OnRender();

            ImGui::Begin("Test");
            if (currentTest != testMenu && ImGui::Button("<-"))
            {
                delete currentTest;
                currentTest = testMenu;
            }
            currentTest->OnImGuiRender();
            ImGui::End();
        }
    }

private:
	GLObject::GLObject *currentTest;
    GLObject::TestMenu *testMenu;
};