#include "PointCloud.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include <utilities/Consts.h>
#include <utilities/Utilities.h>

namespace GLObject
{
    PointCloud::PointCloud(DepthCamera *depthCamera, const Camera *cam) : m_DepthCamera(depthCamera)
    {
        this->camera = cam;
        GLCall(glEnable(GL_BLEND));
        GLCall(glEnable(GL_CULL_FACE));

        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        GLCall(glGetFloatv(GL_COLOR_CLEAR_VALUE, m_ClearColor));

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
                m_Points[i].Position = { Normalisem11((float)h / (float)height),
                                         Normalisem11((float)w / (float)width) };

                m_Points[i].HalfLength = 1.5f / (float)width;
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
        uint16_t maxDepth = 0;

        for (unsigned int h = height - 1; h > 0; --h)
        {
            for (unsigned int w = width; w > 0; --w)
            {
                int i = h * width + w;

                if (depth[i] > maxDepth && depth[i] > m_MaxDepth)
                {
                    maxDepth = depth[i];
                }

                // Read depth data
                m_Points[i].updateDepth(m_Depth_Scale * -1.0f * Normalisem11((float)(depth[i]) / (m_MaxDepth == 0.0f ? (float)m_DepthCamera->getDepthStreamMaxDepth() : m_MaxDepth)), m_Depth_Scale);
                // Copy vertices into vertex array
                memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
            }
        }
        //m_MaxDepth = std::max(m_MaxDepth, (float)maxDepth);
        m_IndexBuffer->Bind();
        
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * numElements * Point::VertexCount, m_Vertices));
    }

    void PointCloud::OnRender()
    {
        GLCall(glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_Translation);
        glm::mat4 mvp = camera->getViewProjection() * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4f("u_MVP", mvp);
        m_Shader->SetUniform1f("u_Scale", m_Scale);

        Renderer renderer;
        renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
    }

    void PointCloud::OnImGuiRender()
    {
        ImGui::SliderFloat3("Translation", &m_Translation.x, -2.0f, 2.0f);
        ImGui::SliderFloat("Depth Scale", &m_Depth_Scale, 0.0f, 10.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.0f, 10.0f);
    }
}