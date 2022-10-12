#include "TestPoint.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace GLObject
{
    TestPoint::TestPoint()
    {
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_VAO = std::make_unique<VertexArray>();
        m_Position = {30.0f, 40.0f};
        m_Depth = 30.0f;
        Point p;
        p.Position = m_Position;
        p.Depth = m_Depth;
        p.HalfLength = 20;
        p.setVertexArray();
        m_VB = std::make_unique<VertexBuffer>(&p.Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<float>(3);
        m_VBL->Push<float>(3);

        m_VAO->AddBuffer(*m_VB, *m_VBL);
        m_IndexBuffer = std::make_unique<IndexBuffer>(Point::getIndices(0), Point::IndexCount);

        m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_Shader->Bind();      

        m_Width = 300;
        m_Height = 300;
        m_MaxDepth = 300;
        m_Proj = glm::ortho(0.0f, (float)m_Width, 0.0f, (float)m_Height, 0.0f, (float)m_MaxDepth);
    }

    void TestPoint::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer renderer;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_ModelTranslation) * glm::rotate(glm::mat4(1.0f), glm::radians(m_RotationFactor), m_Rotation);

        glm::mat4 mvp = m_Proj * m_View * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4f("u_MVP", mvp);
        m_Shader->SetUniform1f("u_Scale", m_Scale);

        renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
    }

    void TestPoint::OnImGuiRender()
    {

        ImGui::Text("Model Translation");
        ImGui::SliderFloat("X", &m_ModelTranslation.x, 0.0f, m_Width);
        ImGui::SliderFloat("Y", &m_ModelTranslation.y, 0.0f, m_Height);
        ImGui::SliderFloat("Z", &m_ModelTranslation.z, 0.0f, m_MaxDepth);

        ImGui::Text("Model Rotation");
        ImGui::SliderFloat("Rotation Factor", &m_RotationFactor, 0.0f, 360.0f);
        ImGui::SliderFloat3("Rotation", &m_Rotation.x, 0.0f, 1.0f);

        ImGui::Text("Point Position");
        ImGui::SliderFloat("X", &m_Position[0], 0.0f, m_Width);
        ImGui::SliderFloat("Y", &m_Position[1], 0.0f, m_Height);
        ImGui::SliderInt("Depth", (int *)&m_Depth, 0.0f, m_MaxDepth);
        ImGui::SliderFloat("Scale", &m_Scale, 0.0f, 10.0f);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
}