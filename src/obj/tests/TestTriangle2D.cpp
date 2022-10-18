#include "TestTriangle2D.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace GLObject
{
    TestTriangle2D::TestTriangle2D() : TestTriangle2D(nullptr) { }
    TestTriangle2D::TestTriangle2D(Camera *cam) : camera(cam)
    {
        float positions[] = {
            -50.0f, -50.0f,  // 0
             50.0f, -50.0f,  // 1
              0.0f,  50.0f,  // 2
        };

        unsigned int indices[] = {
            0, 2, 1
        };

        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_VAO = std::make_unique<VertexArray>();

        m_VB = std::make_unique<VertexBuffer>(positions, 4 * 2 * sizeof(float));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<float>(2);

        m_VAO->AddBuffer(*m_VB, *m_VBL);
        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, 6);

        m_Shader = std::make_unique<Shader>("resources/shaders/basic.shader");
        m_Shader->Bind();
        m_Shader->SetUniform4f("u_Color", m_Color[0], m_Color[1], m_Color[2], m_Color[3]);
    }

    void TestTriangle2D::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer renderer;

        glm::mat4 proj = glm::ortho(m_LeftRightOrtho.x, m_LeftRightOrtho.y,
                                    m_BottomTopOrtho.x, m_BottomTopOrtho.y,
                                    m_NearFarOrtho.x, m_NearFarOrtho.y);
        glm::mat4 view = glm::translate(glm::mat4(1.0f), m_ViewTranslation);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_ModelTranslation);

        glm::mat4 mvp = proj * view * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4f("u_MVP", mvp);
        m_Shader->SetUniform4f("u_Color", m_Color[0], m_Color[1], m_Color[2], m_Color[3]);

        renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
    }

    void TestTriangle2D::OnImGuiRender()
    {
        ImGui::ColorEdit4("Triangle Color", m_Color);   
        ImGui::Text("Projection Ortho: ");
        ImGui::SliderFloat2("Left <-> Right", &m_LeftRightOrtho.x, 0.0f, 960.0f);
        ImGui::SliderFloat2("Bottom <-> Top", &m_BottomTopOrtho.x, 0.0f, 960.0f);
        ImGui::SliderFloat2("zNear <-> zFar", &m_NearFarOrtho.x, 0.0f, 960.0f);

        ImGui::SliderFloat3("View Translation", &m_ViewTranslation.x, 0.0f, 960.0f);
        ImGui::SliderFloat3("Model Translation", &m_ModelTranslation.x, 0.0f, 960.0f);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
}