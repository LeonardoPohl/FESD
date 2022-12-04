#include "PointCloud.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include <ranges>

#define PixIter for(int i = 0; i < m_NumElements; i++)
#define UpdateVertices(i) memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));

namespace GLObject
{
    PointCloud::PointCloud(DepthCamera *depthCamera, const Camera *cam, Renderer *renderer, float metersPerUnit) : mp_DepthCamera(depthCamera), m_MetersPerUnit(metersPerUnit)
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
                m_Points[i].updateVertexArray(1.f, m_CMAP);

                memcpy(indices + i * Point::IndexCount, Point::getIndices(i), Point::IndexCount * sizeof(unsigned int));
            }
        }

        m_GLUtil.mp_Renderer = renderer;
        m_GLUtil.m_VAO = std::make_unique<VertexArray>();

        m_GLUtil.m_VB = std::make_unique<VertexBuffer>(m_NumElements * Point::VertexCount * sizeof(Point::Vertex));
        m_GLUtil.m_VBL = std::make_unique<VertexBufferLayout>();

        m_GLUtil.m_VBL->Push<GLfloat>(3);
        m_GLUtil.m_VBL->Push<GLfloat>(4);

        m_GLUtil.m_VAO->AddBuffer(*m_GLUtil.m_VB, *m_GLUtil.m_VBL);

        m_GLUtil.m_IndexBuffer = std::make_unique<IndexBuffer>(indices, numIndex);

        m_GLUtil.m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_GLUtil.m_Shader->Bind();

        m_Vertices = new Point::Vertex[m_NumElements * Point::VertexCount] {};

        m_PointDistribution = std::make_unique<std::uniform_int_distribution<int>>(0, m_NumElements - 1);
        m_ColorDistribution = std::make_unique<std::uniform_int_distribution<int>>(0, 255);
    }

    void PointCloud::OnUpdate()
    {
        const int16_t *depth;

        if (m_State.m_State == m_State.STREAM)
        {
            depth = static_cast<const int16_t *>(mp_DepthCamera->getDepth());
            PixIter 
            { 
                streamDepth(i, depth);
                UpdateVertices(i)
            }
        }
        else if (m_State.m_State == m_State.NORMALS)
        {
            startNormalCalculation();
            PixIter 
            { 
                calculateNormals(i);
                UpdateVertices(i)
            }
        }
        else if (m_State.m_State == m_State.CELLS)
        {
            startCellAssignment();
            PixIter 
            { 
                assignCells(i);
                UpdateVertices(i)
            }
        }
        else if (m_State.m_State == m_State.CALC_CELLS)
        {
            startCellCalculation();
            PixIter
            { 
                calculateCells(i);
                UpdateVertices(i)
            }
        }
        
        m_GLUtil.m_IndexBuffer->Bind();

        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Point::Vertex) * m_NumElements * Point::VertexCount, m_Vertices));
    }

    void PointCloud::OnRender()
    {
        glm::mat4 model{ 1.0f };
        model = glm::rotate(model, m_GLUtil.m_RotationFactor, m_GLUtil.m_Rotation);
        model = glm::translate(model, m_GLUtil.m_Translation);

        glm::mat4 mvp = camera->getViewProjection() * model;

        m_GLUtil.m_Shader->Bind();
        m_GLUtil.m_Shader->SetUniform1f("u_Scale", m_GLUtil.m_Scale);
        m_GLUtil.m_Shader->SetUniformMat4f("u_MVP", mvp);

        m_GLUtil.mp_Renderer->Draw(*m_GLUtil.m_VAO, *m_GLUtil.m_IndexBuffer, *m_GLUtil.m_Shader);
    }

    void PointCloud::OnImGuiRender()
    {
        m_State.showState();

        if (m_State != m_State.STREAM && ImGui::Button("Resume Stream"))
            resumeStream();

        if (m_State == m_State.STREAM && ImGui::Button("Pause Stream"))
            pauseStream();

        if (m_State != m_State.CELLS && ImGui::Button("Show Cells"))
            startCellAssignment();

        if (m_State != m_State.NORMALS && ImGui::Button("Show Normals"))
            startNormalCalculation();

        if (m_State != m_State.CALC_CELLS && ImGui::Button("Calculate Cells"))
            startCellCalculation();

        if (m_State == m_State.CELLS || m_State == m_State.CALC_CELLS)
        {
            ImGui::Checkbox("Show Average Normals", &m_ShowAverageNormals);
            if (ImGui::SliderInt("Cell devisions", &m_NumCellDevisions, 0, 400))
            {
                m_CellSize = Cell::getCellSize(m_BoundingBox, m_NumCellDevisions);
                m_CellsAssigned = false;
                m_ColorBypCell.clear();
                m_pCellByKey.clear();
                m_PlanarpCells.clear();
                m_NonPlanarpCells.clear();
            }
        }

        if (m_State == m_State.CALC_CELLS) {
            if (ImGui::SliderFloat("Planar Threshold", &m_PlanarThreshold, 0.0001f, 1.0f))
            {
                m_PlanarpCells.clear();
                m_NonPlanarpCells.clear();
                for (auto const &[_, cell] : m_pCellByKey)
                {
                    cell->updateNDTType();

                    if (cell->getType() == Cell::NDT_TYPE::Planar)
                        m_PlanarpCells.push_back(cell);
                    else
                        m_NonPlanarpCells.push_back(cell);
                }
            }
        }

        m_GLUtil.manipulateTranslation();
    }

    void PointCloud::streamDepth(int i, const int16_t *depth)
    {
        int depth_i = m_StreamWidth * (m_StreamHeight + 1) - i;

        if (m_BoundingBox.updateBox(m_Points[i].getPoint()))
        {
            m_CellSize = Cell::getCellSize(m_BoundingBox, m_NumCellDevisions);
        }

        // Read depth data
        m_Points[i].updateVertexArray((float)depth[depth_i] * m_MetersPerUnit, m_CMAP);
    }

    void PointCloud::startNormalCalculation()
    {
        m_State.setState(PointCloudStreamState::NORMALS);
        m_NormalsCalculated = true;
    }

    void PointCloud::calculateNormals(int i)
    {
        auto p = m_Points[i].getPoint();

        int i_y = i;
        int i_x = i;

        if (i == 0)
            i_y += 1;
        else
            i_y -= 1;

        if (i < m_StreamWidth)
            i_x += m_StreamWidth;
        else
            i_x -= m_StreamWidth;

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

        m_State.setState(PointCloudStreamState::CELLS);

        if (!m_CellsAssigned)
        {
            PixIter
            {
                auto p = m_Points[i].getPoint();

                std::string key = Cell::getKey(m_BoundingBox, m_CellSize, p);

                if (!m_pCellByKey.contains(key))
                {
                    glm::vec3 color{ m_ColorDistribution->operator()(m_Generator), 
                                     m_ColorDistribution->operator()(m_Generator), 
                                     m_ColorDistribution->operator()(m_Generator) };
                    m_pCellByKey.insert(std::make_pair(key, new Cell(key, &m_PlanarThreshold)));
                    m_ColorBypCell.insert(std::make_pair(m_pCellByKey[key], color));
                }
                m_pCellByKey[key]->addPoint(&m_Points[i]);
            }

            m_CellsAssigned = true;
        }
    }

    void PointCloud::assignCells(int i)
    {
        auto p = m_Points[i].getPoint();

        std::string key = Cell::getKey(m_BoundingBox, m_CellSize, p);

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
        if (!m_CellsAssigned)
        {
            startCellAssignment();
            //PixIter assignCells(i);
        }

        m_State.setState(PointCloudStreamState::CALC_CELLS);

        if (m_PlanarpCells.empty())
        {
            for (auto const &[_, cell] : m_pCellByKey)
            {
                cell->calculateNDT();

                if (cell->getType() == Cell::NDT_TYPE::Planar)
                    m_PlanarpCells.push_back(cell);
                else
                    m_NonPlanarpCells.push_back(cell);
            }
            m_PointDistribution = std::make_unique<std::uniform_int_distribution<int>>(0, m_PlanarpCells.size());
        }
    }

    void PointCloud::calculateCells(int i)
    {
        auto p = m_Points[i].getPoint();

        std::string key = Cell::getKey(m_BoundingBox, m_CellSize, p);

        glm::vec3 col;

        auto cell = m_pCellByKey[key];
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
            else if (type == Cell::NDT_TYPE::Linear)
                col = glm::vec3{ 1.0f, 0.0f, 0.0f };
            else if (type == Cell::NDT_TYPE::Spherical)
                col = glm::vec3{ 0.0f, 1.0f, 0.0f };
        }

        for (int v : std::views::iota(0, Point::VertexCount))
            m_Points[i].Vertices[v].Color = { col.x,
                                              col.y,
                                              col.z,
                                              1.0f };

    }

    void PointCloud::doPlaneSegmentation()
    {
        std::vector<int> plane_points{};

        plane_points.push_back(m_PointDistribution->operator()(m_Generator));
        
    }
}