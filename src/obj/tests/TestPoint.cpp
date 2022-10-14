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
        m_MaxDepth = 6000;

        m_Position = {0.0f,0.0f};
        m_Depth = 30.0f;

        m_Vertices = new Point::Vertex[2 * Point::VertexCount]{};
        unsigned int *indices = new unsigned int[2 * Point::IndexCount];

        Point *points = new Point[2];

        Point p1;
        p1.Position = m_Position;
        p1.Depth = 2.0f * m_Depth / m_MaxDepth - 1;
        p1.HalfLength = 0.5f;
        p1.setVertexArray();
        memcpy(m_Vertices + 0 * Point::VertexCount, &p1.Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        memcpy(indices + 0 * Point::IndexCount, Point::getIndices(0), Point::IndexCount * sizeof(unsigned int));

        Point p2;
        p2.Position = { 0.5f, -0.2f };
        p2.Depth = 2.0f * m_Depth / m_MaxDepth - 1;
        p2.HalfLength = 0.5f;
        p2.setVertexArray();
        memcpy(m_Vertices + 1 * Point::VertexCount, &p2.Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        memcpy(indices + 1 * Point::IndexCount, Point::getIndices(1), Point::IndexCount * sizeof(unsigned int));

        for (int i = 0; i < 2 * Point::VertexCount; i++)
        {
            if (i == Point::VertexCount)
                std::cout << std::endl;
            std::cout << (m_Vertices + i)->Position[0] << ", ";
            std::cout << (m_Vertices + i)->Position[1] << ", ";
            std::cout << (m_Vertices + i)->Position[2] << std::endl;
        }

        for (int i = 0; i < 2 * Point::IndexCount; i+=3)
        {
            if (i == Point::IndexCount)
                std::cout << std::endl;
            std::cout << *(indices + i + 0) << ", ";
            std::cout << *(indices + i + 1) << ", ";
            std::cout << *(indices + i + 2) << std::endl;
        }

        m_VAO = std::make_unique<VertexArray>();
        m_VB = std::make_unique<VertexBuffer>(m_Vertices, 2 * Point::VertexCount * sizeof(Point::Vertex));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<float>(3);
        m_VBL->Push<float>(3);

        m_VAO->AddBuffer(*m_VB, *m_VBL);

        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, 2 * Point::IndexCount);

        m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_Shader->Bind();      

        m_Proj = glm::perspective(glm::radians(45.0f), (float)m_Width / (float)m_Height, 0.1f, (float)m_MaxDepth);
        //m_Proj = glm::ortho(0.0f, (float)m_Width, 0.0f, (float)m_Height, 0.0f, (float)m_MaxDepth);
    }

    void TestPoint::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer renderer;

        glm::mat4 model = glm::translate(glm::rotate(glm::mat4(1.0f), glm::radians(m_RotationFactor), m_Rotation), m_ModelTranslation);

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
        ImGui::SliderFloat("Z", &m_ModelTranslation.z, -2.0f, 2.0f);

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