#include "PointCloud.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include <utilities/Consts.h>
#include <utilities/Utilities.h>

#include <ranges>

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
        
        m_Distribution = std::make_unique<std::uniform_int_distribution<int>>(0, numElements - 1);
    }

    void PointCloud::OnUpdate()
    {
        if (!doUpdate)
            return;

        const unsigned int width = m_DepthCamera->getDepthStreamWidth();
        const unsigned int height = m_DepthCamera->getDepthStreamHeight();

        const unsigned int numElements = width * height;
        
        // Do switch State

        if (!doFloorDetection)
        {
            auto depth = static_cast<const int16_t *>(m_DepthCamera->getDepth());

            for (unsigned int w = 0; w < width; w++)
            {
                for (unsigned int h = 0; h < height; h++)
                {
                    int point_i = h * width + w;
                    int depth_i = (height - h) * width + (width - w);

                    auto p = m_Points[point_i].getPoint();

                    if (p.x > m_MaxBoundingPoint.x) m_MaxBoundingPoint.x = p.x;
                    if (p.y > m_MaxBoundingPoint.y) m_MaxBoundingPoint.y = p.y;
                    if (p.z > m_MaxBoundingPoint.z) m_MaxBoundingPoint.z = p.z;

                    if (p.x < m_MinBoundingPoint.x) m_MinBoundingPoint.x = p.x;
                    if (p.y < m_MinBoundingPoint.y) m_MinBoundingPoint.y = p.y;
                    if (p.z < m_MinBoundingPoint.z) m_MinBoundingPoint.z = p.z;

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
            float xCellSize = (m_MaxBoundingPoint.x - m_MinBoundingPoint.x) / (float)m_OctTreeDevisions;
            float yCellSize = (m_MaxBoundingPoint.y - m_MinBoundingPoint.y) / (float)m_OctTreeDevisions;
            float zCellSize = (m_MaxBoundingPoint.z - m_MinBoundingPoint.z) / (float)m_OctTreeDevisions;

            std::cout << m_MaxBoundingPoint.x << "-" << m_MinBoundingPoint.x << "=" << (m_MaxBoundingPoint.x - m_MinBoundingPoint.x) << ", ";
            std::cout << m_MaxBoundingPoint.y << "-" << m_MinBoundingPoint.y << "=" << (m_MaxBoundingPoint.y - m_MinBoundingPoint.y) << ", ";
            std::cout << m_MaxBoundingPoint.z << "-" << m_MinBoundingPoint.z << "=" << (m_MaxBoundingPoint.z - m_MinBoundingPoint.z) << std::endl;

            for (int i = 0; i < numElements; i++)
            {
                auto p = m_Points[i].getPoint();

                int x = std::round((p.x - m_MinBoundingPoint.x) / xCellSize);
                int y = std::round((p.y - m_MinBoundingPoint.y) / yCellSize);
                int z = std::round((p.z - m_MinBoundingPoint.z) / zCellSize);

                std::string key = std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
                if (colorByCell.find(key) == colorByCell.end())
                {
                    glm::vec3 color {rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f };
                    colorByCell.insert(std::make_pair( key, color));
                }

                auto col = colorByCell.at(key);

                for (int v : std::views::iota(0, Point::VertexCount))
                    m_Points[i].Vertices[v].Color = { col.x,
                                                      col.y,
                                                      col.z,
                                                      1.0f };

                memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
            }
            m_IndexBuffer->Bind();

            GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) *numElements *Point::VertexCount, m_Vertices));

            /*
            std::vector<int> plane_points{};
            
            plane_points.push_back(m_Distribution->operator()(m_Generator));
            plane_points.push_back(m_Distribution->operator()(m_Generator));
            plane_points.push_back(m_Distribution->operator()(m_Generator));

            Plane p{ 
                m_Points[plane_points[0]].getPoint(),
                m_Points[plane_points[1]].getPoint(),
                m_Points[plane_points[2]].getPoint()
            };

            std::vector<int> points{};

            for (int i : std::views::iota(0, (int)(numElements - 1)))
                if (p.inDistance(m_Points[i].getPoint(), m_DistanceThreshold))
                    points.push_back(i);

            if (m_PointCountThreshold < points.size())
            {
                pointCountByPlane.push_back({ p, points.size() });
                
                maxPointCount = maxPointCount > points.size() ? maxPointCount : points.size();

                std::array<float, 4> randColor{ rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f, 1.0f };

                for (int i : std::views::iota(0, (int)(numElements - 1)))
                {
                    
                    if (std::binary_search(points.begin(), points.end(), i))
                        for (int v : std::views::iota(0, Point::VertexCount))
                            m_Points[i].Vertices[v].Color = randColor;

                    memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
                }

                m_IndexBuffer->Bind();

                GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) *numElements *Point::VertexCount, m_Vertices));
            }*/
        }
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
        if (doFloorDetection)
        {
            ImGui::SliderInt("Oct Tree Devision", &m_OctTreeDevisions, 0, 200);

            ImGui::Text("Number of planes: %d", pointCountByPlane.size());
            ImGui::Text("Max Number of Points: %d", maxPointCount);
            if (ImGui::SliderInt("Point Count Threshold", &m_PointCountThreshold, 0, 100000))
                pointCountByPlane.clear();
            if (ImGui::SliderFloat("Distance Threshold", &m_DistanceThreshold, 0.0f, 10.0f))
                pointCountByPlane.clear();
        }
        else
        {
            ImGui::Checkbox("Update Pointcloud", &doUpdate);
        }
        if (ImGui::Checkbox("Detect floor", &doFloorDetection))
            pointCountByPlane.clear();

        const char *elem_name = Point::CMAP_NAMES[cmap_elem];
        if (ImGui::SliderInt("Color map", &cmap_elem, 0, Point::CMAP_COUNT - 1, elem_name))
        {
            cmap = static_cast<Point::CMAP>(cmap_elem);
        }

        ImGui::SliderAngle("Rotation Factor", &m_RotationFactor);
        ImGui::SliderFloat3("Rotation", &m_Rotation.x, -1.0f, 1.0f);

        ImGui::SliderFloat3("Translation", &m_Translation.x, -2.0f, 2.0f);
        ImGui::SliderFloat("Depth Scale", &m_Depth_Scale, 0.001f, 10.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 10.0f);
    }
}