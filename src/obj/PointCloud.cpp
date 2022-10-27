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
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glDepthFunc(GL_LESS));
        GLCall(glDepthMask(GL_FALSE));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        const int width = m_DepthCamera->getDepthStreamWidth();
        const int height = m_DepthCamera->getDepthStreamHeight();

        const unsigned int numElements = width * height;
        const unsigned int numIndex = numElements * Point::IndexCount;

        m_Points = new Point[numElements];
        auto *indices = new unsigned int[numIndex];

        for (unsigned int w = 0; w < width; w++)
        {
            for (unsigned int h = 0; h < height; h++)
            {
                int i = h * width + w;
                m_Points[i].Position = { Normalisem11((float)w / (float)width) ,
                                         Normalisem11((float)h / (float)height),
                                        };

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
        if (!doUpdate)
            return;

        const unsigned int width = m_DepthCamera->getDepthStreamWidth();
        const unsigned int height = m_DepthCamera->getDepthStreamHeight();

        const unsigned int numElements = width * height;

        auto depth = (int16_t *)m_DepthCamera->getDepth();

        for (unsigned int w = 0; w < width; w++)
        {
            for (unsigned int h = 0; h < height; h++)
            {
                int point_i = h * width + w;
                int depth_i = (height - h) * width + (width - w);

                auto normalised_depth = Normalisem11((float)(depth[depth_i]) / (float)m_DepthCamera->getDepthStreamMaxDepth());
                auto scaled_depth = m_Depth_Scale * -1.0f * normalised_depth;

                // Read depth data
                m_Points[point_i].updateDepth(scaled_depth, m_Depth_Scale, cmap);

                // Copy vertices into vertex array
                memcpy(m_Vertices + point_i * Point::VertexCount, &m_Points[point_i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
            }
        }
        m_IndexBuffer->Bind();
        
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * numElements * Point::VertexCount, m_Vertices));
    }

    void PointCloud::OnRender()
    {
        glm::mat4 model{ 1.0f };
        model = glm::rotate(model, m_RotationFactor, m_Rotation);
        model = glm::translate(model, m_Translation);

        glm::mat4 mvp = camera->getViewProjection() * model;

        m_Shader->Bind();
        m_Shader->SetUniform1f("u_Scale", m_Scale);
        m_Shader->SetUniformMat4f("u_Intrinsics", m_DepthCamera->getIntrinsics());
        m_Shader->SetUniformMat4f("u_MVP", mvp);

        Renderer renderer;
        renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
    }

    void PointCloud::OnImGuiRender()
    {
        ImGui::Checkbox("Update Pointcloud", &doUpdate);

        ImGui::SliderAngle("Rotation Factor", &m_RotationFactor);
        ImGui::SliderFloat3("Rotation", &m_Rotation.x, -1.0f, 1.0f);

        const char *elem_name = Point::CMAP_NAMES[cmap_elem];
        if (ImGui::SliderInt("Color map", &cmap_elem, 0, Point::CMAP_COUNT - 1, elem_name))
        {
            cmap = static_cast<Point::CMAP>(cmap_elem);
        }

        ImGui::SliderFloat3("Translation", &m_Translation.x, -2.0f, 2.0f);
        ImGui::SliderFloat("Depth Scale", &m_Depth_Scale, 0.0f, 10.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.0f, 10.0f);
    }
}