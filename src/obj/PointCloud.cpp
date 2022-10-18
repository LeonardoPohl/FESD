#include "PointCloud.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include <utilities/Consts.h>
#include <utilities/Utilities.h>

namespace GLObject
{
    PointCloud::PointCloud(DepthCamera *depthCamera) : PointCloud(nullptr, depthCamera) { }
    PointCloud::PointCloud(Camera *cam, DepthCamera *depthCamera) : camera(cam), m_DepthCamera(depthCamera)
    {
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        const int width = m_DepthCamera->getDepthStreamWidth();
        const int height = m_DepthCamera->getDepthStreamHeight();

        const unsigned int numElements = width * height;
        const unsigned int numIndex = numElements * Point::IndexCount;

        m_Points = new Point[numElements];
        auto *indices = new unsigned int[numIndex];

        for (unsigned int h = 0; h < height; h++)
        {
            for (unsigned int w = 0; w < width; w++)
            {
                int i = h * width + w;
                m_Points[i].Position = { ((float)h / (float)height),
                                         ((float)w / (float)width) };

                m_Points[i].HalfLength = 0.25f / (float)width;
                m_Points[i].Depth = 0.0f;
                m_Points[i].updateVertexArray();

                memcpy(indices + i * Point::IndexCount, Point::getIndices(i), Point::IndexCount * sizeof(unsigned int));
            }
        }

        m_VAO = std::make_unique<VertexArray>();

        m_VB = std::make_unique<VertexBuffer>(numElements * Point::VertexCount * sizeof(Point::Vertex));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<GLfloat>(3);
        m_VBL->Push<GLfloat>(4);

        m_VAO->AddBuffer(*m_VB, *m_VBL);

        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, numIndex);

        m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_Shader->Bind();

        m_Vertices = new Point::Vertex[numElements * Point::VertexCount] {};
    }

    void PointCloud::OnUpdate()
    {
        auto depth = m_DepthCamera->getDepth();

        const unsigned int height = m_DepthCamera->getDepthStreamWidth();
        const unsigned int width = m_DepthCamera->getDepthStreamHeight();

        const unsigned int numElements = width * height;
        uint8_t maxDepth = 0;

        for (unsigned int h = 0; h < height; h++)
        {
            for (unsigned int w = 0; w < width; w++)
            {
                int i = h * width + w;

                if (depth[i] > maxDepth && depth[i] > m_MaxDepth)
                {
                    maxDepth = depth[i];
                }

                // Read depth data
                m_Points[i].updateDepth(Normalisem11((float)(depth[i]) / (m_MaxDepth == 0.0f ? (float)m_DepthCamera->getDepthStreamMaxDepth() : m_MaxDepth)));
                // Copy vertices into vertex array
                memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
            }
        }
        //m_MaxDepth = std::max(m_MaxDepth, (float)maxDepth);
        m_IndexBuffer->Bind();
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * numElements * Point::VertexCount, m_Vertices));

        // Assigns different transformations to each matrix
        m_Proj = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, -1.0f, 1.0f);
    }

    void PointCloud::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::translate(glm::rotate(glm::mat4(1.0f), m_RotationFactor, m_Rotation), m_Translation);
        glm::mat4 mvp = (camera ? camera->getViewProjection() : m_Proj * m_View) * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4f("u_MVP", mvp);
        m_Shader->SetUniform1f("u_Scale", m_Scale);

        Renderer renderer;
        renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
    }

    void PointCloud::OnImGuiRender()
    {
        ImGui::SliderAngle("Rotation Factor", &m_RotationFactor);
        ImGui::SliderFloat3("Rotation", &m_Rotation.x, 0.0f, 1.0f);
        ImGui::SliderFloat3("Translation", &m_Translation.x, -2.0f, 2.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.0f, 10.0f);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
}