#include "TestPoint.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <utilities/Consts.h>

namespace GLObject
{
    TestPoint::TestPoint()
    {
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_Width = WINDOW_WIDTH;
        m_Height = WINDOW_HEIGHT;
        m_MaxDepth = 500;

        m_VAO = std::make_unique<VertexArray>();
        m_Position = {2.0f * 40.0f/ m_Width - 1, 2.0f * 30.0f/ m_Height - 1};
        m_Depth = 30.0f;
        Point p;
        p.Position = m_Position;
        p.Depth = 2.0f * m_Depth / m_MaxDepth - 1;
        p.HalfLength = 1.0f;
        p.setVertexArray();
        for (int i = 0; i < Point::VertexCount; i ++)
        {
            std::cout << p.Vertices[i].Position[0] << ", ";
            std::cout << p.Vertices[i].Position[1] << ", ";
            std::cout << p.Vertices[i].Position[2] << std::endl;
        }
        m_VB = std::make_unique<VertexBuffer>(&p.Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<float>(3);
        m_VBL->Push<float>(3);

        m_VAO->AddBuffer(*m_VB, *m_VBL);

        auto indices = Point::getIndices(0);
        for (int i = 0; i < p.IndexCount; i += 3)
        {
            std::cout << *(indices + i);
            std::cout << *(indices + i + 1);
            std::cout << *(indices + i + 2) << std::endl;
        }
        std::cout << *indices << std::endl;
        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, Point::IndexCount);

        m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_Shader->Bind();      

        m_Proj = glm::perspective(glm::radians(45.0f), (float)m_Width / m_Height, 0.1f, (float)m_MaxDepth);
        //m_Proj = glm::ortho(0.0f, (float)m_Width, 0.0f, (float)m_Height, 0.0f, (float)m_MaxDepth);
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
        ImGui::SliderFloat("X", &m_ModelTranslation.x, -1.0f, 1.0f);
        ImGui::SliderFloat("Y", &m_ModelTranslation.y, -1.0f, 1.0f);
        ImGui::SliderFloat("Z", &m_ModelTranslation.z, -1.0f, 1.0f);

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