#include "PointCloud.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include <utilities/Consts.h>
#include <utilities/Utilities.h>

#include <ranges>

namespace GLObject
{
    const char *PointCloud::StateNames[] = { "Stream", "Idle", "Show Normals", "Show Cells", "Calculate Cells" };

    PointCloud::PointCloud(DepthCamera *depthCamera, const Camera *cam) : m_DepthCamera(depthCamera)
    {
        this->camera = cam;
        GLCall(glEnable(GL_BLEND));
        GLCall(glEnable(GL_CULL_FACE));
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glDepthFunc(GL_LESS));
        GLCall(glDepthMask(GL_FALSE));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_StreamWidth = m_DepthCamera->getDepthStreamWidth();
        m_StreamHeight = m_DepthCamera->getDepthStreamHeight();
        m_NumElements = m_StreamWidth * m_StreamHeight;

        float fx = m_DepthCamera->getIntrinsics(INTRINSICS::FX);
        float fy = m_DepthCamera->getIntrinsics(INTRINSICS::FY);
        float cx = m_DepthCamera->getIntrinsics(INTRINSICS::CX);
        float cy = m_DepthCamera->getIntrinsics(INTRINSICS::CY);

        const unsigned int numIndex = m_NumElements * Point::IndexCount;

        m_Points = new Point[m_NumElements];

        auto *indices = new unsigned int[numIndex];

        for (unsigned int w = 0; w < m_StreamWidth; w++)
        {
            for (unsigned int h = 0; h < m_StreamHeight; h++)
            {
                int i = h * m_StreamWidth + w;

                m_Points[i].PositionFunction = { ((float)w - cx) / fx,
                                                 ((float)h - cy) / fy };

                m_Points[i].HalfLengthFun = 1 / fy;
                m_Points[i].Scale = m_StreamWidth;
                m_Points[i].updateVertexArray(0.1f, m_Depth_Scale / (float)m_DepthCamera->getDepthStreamMaxDepth(), cmap);

                memcpy(indices + i * Point::IndexCount, Point::getIndices(i), Point::IndexCount * sizeof(unsigned int));
            }
        }

        m_VAO = std::make_unique<VertexArray>();

        m_VB = std::make_unique<VertexBuffer>(m_NumElements * Point::VertexCount * sizeof(Point::Vertex));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<GLfloat>(3);
        m_VBL->Push<GLfloat>(4);

        m_VAO->AddBuffer(*m_VB, *m_VBL);

        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, numIndex);

        m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_Shader->Bind();

        m_Vertices = new Point::Vertex[m_NumElements * Point::VertexCount] {};
        
        m_Distribution = std::make_unique<std::uniform_int_distribution<int>>(0, m_NumElements - 1);
    }

    void PointCloud::OnUpdate()
    {
        switch (state)
        {
            case STREAM:
                streamDepth();
                break;
            case IDLE:
                break;
            case NORMALS:
                calculateNormals();
                break;
            case CELLS:
                assignCells();
                break;
            case CALC_CELLS:
                calculateCells();
                break;
            default:
                break;
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
        ImGui::BeginDisabled();
        const char *state_name = StateNames[state_elem];
        if (ImGui::SliderInt("State", &state_elem, 0, StateCount - 1, state_name))
            cmap = static_cast<Point::CMAP>(state_elem);
        ImGui::EndDisabled();

        if (state != STREAM && ImGui::Button("Resume Stream"))
            resumeStream();

        if (state == STREAM && ImGui::Button("Pause Stream"))
            pauseStream();

        if (state != CELLS && ImGui::Button("Show Cells"))
            assignCells();

        if (state != NORMALS && ImGui::Button("Show Normals"))
            calculateNormals();

        if (state != CALC_CELLS && ImGui::Button("Calculate Cells"))
            calculateCells();

        if (state == CELLS)
        {
            ImGui::Checkbox("Show Average Normals", &m_ShowAverageNormals);
            if (ImGui::SliderInt("Chunk devisions", &m_OctTreeDevisions, 0, 400))
            {
                m_CellsAssigned = false;
                colorByCell.clear();
                cellByKey.clear();
            }
        }

        if (ImGui::CollapsingHeader("Translation"))
        {
            ImGui::SliderAngle("Rotation Factor", &m_RotationFactor);
            ImGui::SliderFloat3("Rotation", &m_Rotation.x, -1.0f, 1.0f);

            ImGui::SliderFloat3("Translation", &m_Translation.x, -2.0f, 2.0f);
        }

        if (ImGui::CollapsingHeader("Scale"))
        {
            ImGui::SliderFloat("Depth Scale", &m_Depth_Scale, 0.001f, 30.0f);
            ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 10.0f);
        }
    }

    void PointCloud::streamDepth()
    {
        auto depth = static_cast<const int16_t *>(m_DepthCamera->getDepth());

        for (int i = 1; i < m_NumElements; i++)
        {
            int depth_i = m_StreamWidth * (m_StreamHeight + 1) - i;

            auto p = m_Points[i].getPoint();

            if (p.x > m_MaxBoundingPoint.x) m_MaxBoundingPoint.x = p.x;
            if (p.y > m_MaxBoundingPoint.y) m_MaxBoundingPoint.y = p.y;
            if (p.z > m_MaxBoundingPoint.z) m_MaxBoundingPoint.z = p.z;

            if (p.x < m_MinBoundingPoint.x) m_MinBoundingPoint.x = p.x;
            if (p.y < m_MinBoundingPoint.y) m_MinBoundingPoint.y = p.y;
            if (p.z < m_MinBoundingPoint.z) m_MinBoundingPoint.z = p.z;

            // Read depth data
            m_Points[i].updateVertexArray(depth[depth_i], m_Depth_Scale / (float)m_DepthCamera->getDepthStreamMaxDepth(), cmap);

            // Copy vertices into vertex array
            memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        }
        m_IndexBuffer->Bind();

        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * m_NumElements * Point::VertexCount, m_Vertices));
    }

    void PointCloud::calculateNormals()
    {
        state = NORMALS;
        state_elem = 2;
        m_NormalsCalculated = true;

        for (int i = 1; i < m_NumElements; i++)
        {
            auto p = m_Points[i].getPoint();

            int i_y = i - 1;
            int i_x = i;

            if (i < m_StreamWidth)
            {
                i_x += m_StreamWidth;
            }
            else
            {
                i_x -= m_StreamWidth;
            }

            auto p1 = m_Points[i_y].getPoint();
            auto p2 = m_Points[i_x].getPoint();

            auto normal = m_Points[i].calculateNormal(p1, p2);

            for (int v : std::views::iota(0, Point::VertexCount))
                m_Points[i].Vertices[v].Color = { (normal.x + 1) / 2.0f,
                                                  (normal.y + 1) / 2.0f,
                                                  (normal.z + 1) / 2.0f,
                                                  1.0f };

            memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        }
        m_IndexBuffer->Bind();

        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * m_NumElements * Point::VertexCount, m_Vertices));
    }

    void PointCloud::assignCells()
    {
        if (!m_NormalsCalculated)
            calculateNormals();
        
        state = CELLS;
        state_elem = 3; 
        
        float xCellSize = (m_MaxBoundingPoint.x - m_MinBoundingPoint.x) / (float)m_OctTreeDevisions;
        float yCellSize = (m_MaxBoundingPoint.y - m_MinBoundingPoint.y) / (float)m_OctTreeDevisions;
        float zCellSize = (m_MaxBoundingPoint.z - m_MinBoundingPoint.z) / (float)m_OctTreeDevisions;
        
        if (!m_CellsAssigned)
        {
            for (int i = 0; i < m_NumElements; i++)
            {
                auto p = m_Points[i].getPoint();

                int x = std::round((p.x - m_MinBoundingPoint.x) / xCellSize);
                int y = std::round((p.y - m_MinBoundingPoint.y) / yCellSize);
                int z = std::round((p.z - m_MinBoundingPoint.z) / zCellSize);

                std::string key = std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);

                if (cellByKey.find(key) == cellByKey.end())
                {
                    glm::vec3 color{ rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f };
                    cellByKey.insert(std::make_pair(key, new Cell({x, y, z})));
                    colorByCell.insert(std::make_pair(cellByKey[key], color));
                }
                cellByKey[key]->addPoint(&m_Points[i]);
            }
            
            m_CellsAssigned = true;
        }

        for (int i = 0; i < m_NumElements; i++)
        {
            auto p = m_Points[i].getPoint();

            int x = std::round((p.x - m_MinBoundingPoint.x) / xCellSize);
            int y = std::round((p.y - m_MinBoundingPoint.y) / yCellSize);
            int z = std::round((p.z - m_MinBoundingPoint.z) / zCellSize);

            std::string key = std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);

            glm::vec3 col;

            if (m_ShowAverageNormals)
            {
                auto normal = cellByKey[key]->getNormalisedNormal();
                col = (normal + glm::vec3(1.0f)) / glm::vec3(2.0f);
            }
            else
            {
                col = colorByCell[cellByKey[key]];
            }

            for (int v : std::views::iota(0, Point::VertexCount))
                m_Points[i].Vertices[v].Color = { col.x,
                                                  col.y,
                                                  col.z,
                                                  1.0f };

            memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        }
        m_IndexBuffer->Bind();

        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * m_NumElements * Point::VertexCount, m_Vertices));
    }

    void PointCloud::calculateCells()
    {
        state = CALC_CELLS;
        state_elem = 4;

        assignCells();
        calculateCells();
    }

    /*
    void PointCloud::randomSample()
    {
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
        }
    }*/
}