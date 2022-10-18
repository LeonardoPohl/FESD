#include "TestPoint.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <utilities/Consts.h>

namespace GLObject
{
    TestPoint::TestPoint() : TestPoint(nullptr) { }
    TestPoint::TestPoint(Camera* cam) : camera(cam)
    {
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        const unsigned int numElements = 2;
        const unsigned int numIndex = numElements * Point::IndexCount;

        m_Position = {0.0f, 0.0f};
        m_Depth = 0.0f;
        m_Points = new Point[numElements];
        
        m_Points[0].Position = m_Position;
        m_Points[0].HalfLength = 1.0f / WINDOW_WIDTH;
        m_Points[0].updateVertexArray();
        m_Points[0].updateDepth(m_Depth);

        m_Points[1].Position = { 0.5f, -0.5f };
        m_Points[1].HalfLength = 2.0f / WINDOW_WIDTH;
        m_Points[1].Depth = -0.5f;
        m_Points[1].updateVertexArray();


        m_Vertices = new Point::Vertex[2 * Point::VertexCount]{};
        unsigned int *indices = new unsigned int[numIndex];

        memcpy(indices + 0 * Point::IndexCount, Point::getIndices(0), Point::IndexCount * sizeof(unsigned int));
        memcpy(indices + 1 * Point::IndexCount, Point::getIndices(1), Point::IndexCount * sizeof(unsigned int));

        m_VAO = std::make_unique<VertexArray>();
        m_VB = std::make_unique<VertexBuffer>(2 * Point::VertexCount * sizeof(Point::Vertex));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<float>(3);
        m_VBL->Push<float>(3);

        m_VAO->AddBuffer(*m_VB, *m_VBL);

        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, 2 * Point::IndexCount);

        m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_Shader->Bind();      

        m_Proj = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, -1.0f, 1.0f);
        //m_Proj = glm::ortho(0.0f, (float)m_Width, 0.0f, (float)m_Height, 0.0f, (float)m_MaxDepth);
    }

    void TestPoint::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_Points[0].Position = m_Position;
        m_Points[0].Depth = m_Depth;
        m_Points[0].updateVertexArray();
        m_Points[1].Depth = -0.5f;
        m_Points[0].updateVertexArray();

        memcpy(m_Vertices + 0 * Point::VertexCount, &m_Points[0].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        memcpy(m_Vertices + 1 * Point::VertexCount, &m_Points[1].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));

        m_IndexBuffer->Bind();
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * 2 * Point::VertexCount, m_Vertices));

        glm::mat4 model = glm::translate(glm::rotate(glm::mat4(1.0f), glm::radians(m_RotationFactor), m_Rotation), m_ModelTranslation);
        glm::mat4 mvp = (camera ? camera->getViewProjection() : m_Proj * m_View) * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4f("u_MVP", mvp);
        m_Shader->SetUniform1f("u_Scale", m_Scale);

        Renderer renderer;
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
        ImGui::SliderFloat("X Pos", &m_Position[0], -1.0f, 1.0f);
        ImGui::SliderFloat("Y Pos", &m_Position[1], -1.0f, 1.0f);
        ImGui::SliderFloat("Depth", &m_Depth, -1.0f, 1.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.0f, 10.0f);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
}