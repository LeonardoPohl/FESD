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

        float fx = m_DepthCamera->getIntrinsics(INTRINSICS::FX);
        float fy = m_DepthCamera->getIntrinsics(INTRINSICS::FY);
        float cx = m_DepthCamera->getIntrinsics(INTRINSICS::CX);
        float cy = m_DepthCamera->getIntrinsics(INTRINSICS::CY);

        const unsigned int numElements = width * height;
        const unsigned int numIndex = numElements * Point::IndexCount;

        m_Points = new Point[numElements];

        auto *indices = new unsigned int[numIndex];

        for (unsigned int w = 0; w < width; w++)
        {
            for (unsigned int h = 0; h < height; h++)
            {
                int i = h * width + w;

                m_Points[i].PositionFunction = { ((float)w - cx) / fx,
                                                 ((float)h - cy) / fy };

                m_Points[i].HalfLength = 1.5f;
                m_Points[i].Scale = width;
                m_Points[i].updateVertexArray(0.1f, m_Depth_Scale / (float)m_DepthCamera->getDepthStreamMaxDepth(), cmap);

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
        
        auto depth = static_cast<const int16_t *>(m_DepthCamera->getDepth());

        if (!doFloorDetection)
        {
            for (unsigned int w = 0; w < width; w++)
            {
                for (unsigned int h = 0; h < height; h++)
                {
                    int point_i = h * width + w;
                    int depth_i = (height - h) * width + (width - w);

                    // Read depth data
                    m_Points[point_i].updateVertexArray(depth[depth_i], m_Depth_Scale / (float)m_DepthCamera->getDepthStreamMaxDepth(), cmap);

                    // Copy vertices into vertex array
                    memcpy(m_Vertices + point_i * Point::VertexCount, &m_Points[point_i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
                }
            }
            m_IndexBuffer->Bind();

            GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * numElements * Point::VertexCount, m_Vertices));
        }
        else
        {
            // While Plane with more points is not found
            // Select 3 Points at random (?) that were not part of a plane before
            // 
            // Create a Plane using those Points
            // 
            // iterate all points and check if they are part of plane & count
            // (Extra, color points)
            // 
            // Store Plane with number of matches in map
        }
        // Add plane display mode, where planes with points are shown
    }

    void PointCloud::OnRender()
    {
        glm::mat4 model{ 1.0f };
        model = glm::rotate(model, m_RotationFactor, m_Rotation);
        model = glm::translate(model, m_Translation);

        glm::mat4 mvp = camera->getViewProjection() * model;

        m_Shader->Bind();
        m_Shader->SetUniform1f("u_Scale", m_Scale);
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
        ImGui::SliderFloat("Depth Scale", &m_Depth_Scale, 0.001f, 10.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 10.0f);
    }
}