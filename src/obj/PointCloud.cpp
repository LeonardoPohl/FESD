#include "PointCloud.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include <utilities/Consts.h>
#include <utilities/Utilities.h>

#include <ranges>

namespace GLObject
{
    const char *PointCloud::m_StateNames[] = { "Stream", "Idle", "Show Normals", "Show Cells", "Calculate Cells" };

    PointCloud::PointCloud(DepthCamera *depthCamera, const Camera *cam, Renderer *renderer) : mp_Renderer(renderer), mp_DepthCamera(depthCamera)
    {
        this->camera = cam;
        GLCall(glEnable(GL_BLEND));
        GLCall(glEnable(GL_CULL_FACE));
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glDepthFunc(GL_LESS));
        GLCall(glDepthMask(GL_FALSE));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_StreamWidth = mp_DepthCamera->getDepthStreamWidth();
        m_StreamHeight = mp_DepthCamera->getDepthStreamHeight();
        m_NumElements = m_StreamWidth * m_StreamHeight;

        float fx = mp_DepthCamera->getIntrinsics(INTRINSICS::FX);
        float fy = mp_DepthCamera->getIntrinsics(INTRINSICS::FY);
        float cx = mp_DepthCamera->getIntrinsics(INTRINSICS::CX);
        float cy = mp_DepthCamera->getIntrinsics(INTRINSICS::CY);

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

                m_Points[i].HalfLengthFun = 0.5f / fy;
                m_Points[i].Scale = m_StreamWidth;
                m_Points[i].updateVertexArray(0.1f, m_DepthScale / (float)mp_DepthCamera->getDepthStreamMaxDepth(), m_CMAP);

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
        const int16_t *depth;
        glm::vec3 cellSize;

        if (m_State == STREAM)
        {
            depth = static_cast<const int16_t *>(mp_DepthCamera->getDepth());
        }
        else if (m_State == NORMALS)
        {
            startNormalCalculation();
        }


        for (int i = 0; i < m_NumElements; i++)
        {
            switch (m_State)
            {
                case STREAM:
                    streamDepth(i, depth);
                    break;
                case IDLE:
                    break;
                case NORMALS:
                    calculateNormals(i);
                    break;
                case CELLS:
                    assignCells(i, cellSize);
                    break;
                case CALC_CELLS:
                    calculateCells(i, cellSize);
                    break;
                default:
                    break;
            }

            // Copy vertices into vertex array
            memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
        }
        

        m_IndexBuffer->Bind();

        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * m_NumElements * Point::VertexCount, m_Vertices));
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

        mp_Renderer->Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
    }

    void PointCloud::OnImGuiRender()
    {
        ImGui::BeginDisabled();
        const char *state_name = m_StateNames[m_StateElem];
        if (ImGui::SliderInt("State", &m_StateElem, 0, m_StateCount - 1, state_name))
            m_State = static_cast<State>(m_StateElem);
        ImGui::EndDisabled();

        if (m_State != STREAM && ImGui::Button("Resume Stream"))
            resumeStream();

        if (m_State == STREAM && ImGui::Button("Pause Stream"))
            pauseStream();

        if (m_State != CELLS && ImGui::Button("Show Cells"))
            startCellAssignment();

        if (m_State != NORMALS && ImGui::Button("Show Normals"))
            startNormalCalculation();

        if (m_State != CALC_CELLS && ImGui::Button("Calculate Cells"))
            startCellCalculation();

        if (m_State == CELLS || m_State == CALC_CELLS)
        {
            ImGui::Checkbox("Show Average Normals", &m_ShowAverageNormals);
            if (ImGui::SliderInt("Chunk devisions", &m_OctTreeDevisions, 0, 400))
            {
                m_CellsAssigned = false;
                m_ColorBypCell.clear();
                m_pCellByKey.clear();
                mp_PlanarCells.clear();
                mp_NonPlanarPoints.clear();
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
            ImGui::SliderFloat("Depth Scale", &m_DepthScale, 0.001f, 30.0f);
            ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 10.0f);
        }
    }

    void PointCloud::streamDepth(int i, const int16_t *depth)
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
        m_Points[i].updateVertexArray(depth[depth_i], m_DepthScale / (float)mp_DepthCamera->getDepthStreamMaxDepth(), m_CMAP);
    }

    void PointCloud::startNormalCalculation()
    {
        m_State = NORMALS;
        m_StateElem = 2;

        m_NormalsCalculated = true;
    }

    void PointCloud::calculateNormals(int i)
    {
        auto p = m_Points[i].getPoint();

        int i_y = i;
        int i_x = i;

        if (i == 0)
        {
            i_y += 1;
        }
        else
        {
            i_y -= i;
        }

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
    }

    void PointCloud::startCellAssignment()
    {
        if (!m_NormalsCalculated)
            for (int i = 0; i < m_NumElements; i++)
                calculateNormals(i);

        m_State = CELLS;
        m_StateElem = 3;

        auto cellSize = Cell::getCellSize(m_MinBoundingPoint, m_MaxBoundingPoint, m_OctTreeDevisions);

        if (!m_CellsAssigned)
        {
            for (int i = 0; i < m_NumElements; i++)
            {
                auto p = m_Points[i].getPoint();

                std::string key = Cell::getKey(m_MinBoundingPoint, cellSize, p);

                if (m_pCellByKey.find(key) == m_pCellByKey.end())
                {
                    glm::vec3 color{ rand() % 255 / 255.f, rand() % 255 / 255.f, rand() % 255 / 255.f };
                    m_pCellByKey.insert(std::make_pair(key, new Cell(key)));
                    m_ColorBypCell.insert(std::make_pair(m_pCellByKey[key], color));
                }
                m_pCellByKey[key]->addPoint(&m_Points[i]);
            }

            m_CellsAssigned = true;
        }
    }

    void PointCloud::assignCells(int i, glm::vec3 cellSize)
    {
        auto p = m_Points[i].getPoint();

        std::string key = Cell::getKey(m_MinBoundingPoint, cellSize, p);

        glm::vec3 col;

        if (m_ShowAverageNormals)
        {
            auto normal = m_pCellByKey[key]->getNormalisedNormal();
            col = (normal + glm::vec3(1.0f)) / glm::vec3(2.0f);
        }
        else
        {
            col = m_ColorBypCell[m_pCellByKey[key]];
        }

        for (int v : std::views::iota(0, Point::VertexCount))
            m_Points[i].Vertices[v].Color = { col.x,
                                                col.y,
                                                col.z,
                                                1.0f };
    }

    void PointCloud::startCellCalculation()
    {
        m_State = CALC_CELLS;
        m_StateElem = 4;

        if (!m_CellsAssigned)
        {
            auto cellSize = Cell::getCellSize(m_MinBoundingPoint, m_MaxBoundingPoint, m_OctTreeDevisions);

            assignCells(i, cellSize);
        }

        if (planarCells.empty())
        {
            for (auto const &[_, cell] : cellByKey)
            {
                cell->calculateNDT();

                if (cell->getType() == Cell::NDT_TYPE::Planar)
                    planarCells.push_back(cell);
                else
                    nonPlanarPoints.insert(nonPlanarPoints.end(), cell->getPoints().begin(), cell->getPoints().end());
            }
        }
    }

    void PointCloud::calculateCells(glm::vec3 cellSize, int i)
    {
        auto p = m_Points[i].getPoint();

        std::string key = Cell::getKey(m_MinBoundingPoint, cellSize, p);

        glm::vec3 col;

        auto cell = cellByKey[key];
        auto type = cell->getType();

        if (m_ShowAverageNormals)
        {
            auto normal = cell->getNormalisedNormal();
            col = (normal + glm::vec3(1.0f)) / glm::vec3(2.0f);
        }
        else
        {
            if (type == Cell::NDT_TYPE::Planar)
                col = glm::vec3{ 0.0f, 0.0f, 1.0f };
            else
                col = glm::vec3{ 1.0f, 0.0f, 0.0f };
        }

        for (int v : std::views::iota(0, Point::VertexCount))
            m_Points[i].Vertices[v].Color = { col.x,
                                                col.y,
                                                col.z,
                                                1.0f };

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