#include "PointCloud.h"

#include <iostream>
#include <ranges>
#include <random>

#include <GLCore/GLErrorManager.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#define PixIter(cam_index) for(int i = 0; i < m_NumElements[cam_index]; i++)
#define UpdateVertices(cam_index, i) memcpy(m_Vertices[cam_index] + i, &m_Points[cam_index][i].Vert, sizeof(Point::Vertex));

namespace GLObject
{
    PointCloud::PointCloud(std::vector<DepthCamera *> depthCameras, const Camera *cam, Renderer *renderer) : m_DepthCameras(depthCameras), m_CameraCount(depthCameras.size())
    {
        this->camera = cam;


        GLUtil::setFlags();

        m_NumElementsTotal = 0;

        for (auto cam : m_DepthCameras) {
            m_Rotation.push_back({ });
            m_Translation.push_back({ });

            m_StreamWidths.push_back(cam->getDepthStreamWidth());
            m_StreamHeights.push_back(cam->getDepthStreamHeight());

            m_NumElements.push_back(m_StreamWidths.back() * m_StreamHeights.back());
            m_ElementOffset.push_back(m_NumElementsTotal);
            
            m_MVPS.push_back({ });

            m_Points.push_back(new Point[m_NumElements.back()]{});
            m_Vertices.push_back(new Point::Vertex[m_NumElements.back()]{});

            m_ColorBypCell.push_back({ });
            m_pCellByKey.push_back({ });

            m_PlanarpCells.push_back({ });
            m_NonPlanarpCells.push_back({ });

            m_BoundingBoxes.push_back({ });
            m_CellSizes.push_back({ });

            m_NumElementsTotal += m_NumElements.back();
        }
        
        // Add the last element + 1 so termination criteria is simpler
        m_ElementOffset.push_back(m_NumElementsTotal + 1);
        auto* indices = new unsigned int[m_NumElementsTotal];

        for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
            float fx = m_DepthCameras[cam_index]->getIntrinsics(INTRINSICS::FX);
            float fy = m_DepthCameras[cam_index]->getIntrinsics(INTRINSICS::FY);
            float cx = m_DepthCameras[cam_index]->getIntrinsics(INTRINSICS::CX);
            float cy = m_DepthCameras[cam_index]->getIntrinsics(INTRINSICS::CY);

            for (int w = 0; w < m_StreamWidths[cam_index]; w++) {
                for (int h = 0; h < m_StreamHeights[cam_index]; h++) {
                    int i = h * m_StreamWidths[cam_index] + w;
                    
                    m_Points[cam_index][i].PositionFunction = { ((float)w - cx) / fx,
                                                                ((float)h - cy) / fy };

                    m_Points[cam_index][i].HalfLengthFun = 0.5f / fy;
                    m_Points[cam_index][i].updateVertexArray(1.f, cam_index);
                    indices[i + m_ElementOffset[cam_index]] = i + m_ElementOffset[cam_index];
                }
            }
        }

        m_GLUtil.mp_Renderer = renderer;
        m_GLUtil.m_VAO = std::make_unique<VertexArray>(m_NumElementsTotal);

        m_GLUtil.m_VB = std::make_unique<VertexBuffer>(m_NumElementsTotal * sizeof(Point::Vertex));
        m_GLUtil.m_VBL = std::make_unique<VertexBufferLayout>();

        m_GLUtil.m_VBL->Push<GLfloat>(3);
        m_GLUtil.m_VBL->Push<GLfloat>(3);
        m_GLUtil.m_VBL->Push<GLint>(1);
        
        m_GLUtil.m_VAO->AddBuffer(*m_GLUtil.m_VB, *m_GLUtil.m_VBL);

        m_GLUtil.m_IndexBuffer = std::make_unique<IndexBuffer>(indices, m_NumElementsTotal);

        m_GLUtil.m_Shader = std::make_unique<Shader>("resources/shaders/PointCloud");
        m_GLUtil.m_Shader->Bind();

        m_PointDistribution = std::make_unique<std::uniform_int_distribution<int>>(0, m_NumElementsTotal - 1);
        m_ColorDistribution = std::make_unique<std::uniform_int_distribution<int>>(0, 255);
    }

    void PointCloud::OnUpdate(int currentPlaybackFrame)
    {
        const int16_t *depth;

        for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
            auto cam = m_DepthCameras[cam_index];
            if (!cam->m_IsEnabled)
                continue;

            if (m_State.m_State == m_State.STREAM) {
                depth = static_cast<const int16_t*>(cam->getDepth());
                if (depth != nullptr) {
                    PixIter(cam_index) 
                    {
                        streamDepth(i, cam_index, depth);
                        UpdateVertices(cam_index, i)
                    }
                }
            }
            else if (m_State.m_State == m_State.NORMALS) {
                startNormalCalculation();
                PixIter(cam_index) 
                {
                    calculateNormals(i, cam_index);
                    UpdateVertices(cam_index, i)
                }
            }
            else if (m_State.m_State == m_State.CELLS) {
                startCellAssignment();
                PixIter(cam_index)
                {
                    assignCells(i, cam_index);
                    UpdateVertices(cam_index, i)
                }
            }
            else if (m_State.m_State == m_State.CALC_CELLS) {
                startCellCalculation();
                PixIter(cam_index)
                {
                    calculateCells(i, cam_index);
                    UpdateVertices(cam_index, i)
                }
            }

            GLCall(glBufferSubData(GL_ARRAY_BUFFER, sizeof(Point::Vertex) * m_ElementOffset[cam_index], sizeof(Point::Vertex) * m_NumElements[cam_index], m_Vertices[cam_index]));
        }
    }

    void PointCloud::OnRender()
    {
        m_GLUtil.m_Shader->Bind();

        for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
            glm::mat4 model{ 1.0f };

            if (m_Rotation[cam_index].x != 0 || m_Rotation[cam_index].y != 0 || m_Rotation[cam_index].z != 0)
                model = glm::rotate(model, 1.0f, m_Rotation[cam_index]);
            
            model = glm::translate(model, m_Translation[cam_index]);

            m_GLUtil.m_Shader->SetUniformMat4f(("u_Models[" + std::to_string(cam_index) + "]"), model);
        }

        m_GLUtil.m_Shader->SetUniformMat4f("u_VP", camera->getViewProjection());
        m_GLUtil.mp_Renderer->DrawPoints(*m_GLUtil.m_VAO, *m_GLUtil.m_IndexBuffer, *m_GLUtil.m_Shader);
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

        if (m_State == m_State.CELLS || m_State == m_State.CALC_CELLS) {
            ImGui::Checkbox("Show Average Normals", &m_ShowAverageNormals);
            if (ImGui::SliderInt("Cell devisions", &m_NumCellDevisions, 0, 400))
            {
                for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
                    m_CellSizes[cam_index] = Cell::getCellSize(m_BoundingBoxes[cam_index], m_NumCellDevisions);
                    m_ColorBypCell[cam_index].clear();
                    m_pCellByKey[cam_index].clear();
                    m_PlanarpCells[cam_index].clear();
                    m_NonPlanarpCells[cam_index].clear();
                }
                m_CellsAssigned = false;
            }
        }

        if (m_State == m_State.CALC_CELLS) {
            if (ImGui::SliderFloat("Planar Threshold", &m_PlanarThreshold, 0.0001f, 1.0f)) {
                for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
                    m_PlanarpCells[cam_index].clear();
                    m_NonPlanarpCells[cam_index].clear();

                    for (auto const& [_, cell] : m_pCellByKey[cam_index]) {
                        cell->updateNDTType();

                        if (cell->getType() == Cell::NDT_TYPE::Planar)
                            m_PlanarpCells[cam_index].push_back(cell);
                        else
                            m_NonPlanarpCells[cam_index].push_back(cell);
                    }
                }
            }
        }

        manipulateTranslation();
    }

    void PointCloud::pauseStream()
    {
        m_State.setState(PointCloudStreamState::IDLE);
    }

    void PointCloud::resumeStream()
    {
        m_State.setState(PointCloudStreamState::STREAM);
        for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
            for (auto key : m_pCellByKey[cam_index])
                delete key.second;

            m_pCellByKey[cam_index].clear();
            m_ColorBypCell[cam_index].clear();
            m_PlanarpCells[cam_index].clear();
            m_NonPlanarpCells[cam_index].clear();
        }

        m_CellsAssigned = false;
        m_ShowAverageNormals = false;
        m_NormalsCalculated = false;
    }

    void PointCloud::streamDepth(int i, int cam_index, const int16_t *depth)
    {
        int depth_i = m_StreamWidths[cam_index] * (m_StreamHeights[cam_index] + 1) - i;

        if (m_BoundingBoxes[cam_index].updateBox(m_Points[cam_index][i].getPoint())) {
            m_CellSizes[cam_index] = Cell::getCellSize(m_BoundingBoxes[cam_index], m_NumCellDevisions);
        }

        // Read depth data
        m_Points[cam_index][i].updateVertexArray((float)depth[depth_i] * m_DepthCameras[cam_index]->getMetersPerUnit(), cam_index);
    }

    void PointCloud::startNormalCalculation()
    {
        m_State.setState(PointCloudStreamState::NORMALS);
        m_NormalsCalculated = true;
    }

    void PointCloud::calculateNormals(int i, int cam_index)
    {
        auto p = m_Points[cam_index][i].getPoint();

        int i_y = i;
        int i_x = i;

        if (i == 0)
            i_y += 1;
        else
            i_y -= 1;

        if (i < m_StreamWidths[cam_index])
            i_x += m_StreamWidths[cam_index];
        else
            i_x -= m_StreamWidths[cam_index];

        auto p1 = m_Points[cam_index][i_y].getPoint();
        auto p2 = m_Points[cam_index][i_x].getPoint();

        auto normal = m_Points[cam_index][i].getNormal(p1, p2);

        m_Points[cam_index][i].Vert.Color = { (normal.x + 1) / 2.0f,
                                              (normal.y + 1) / 2.0f,
                                              (normal.z + 1) / 2.0f};
    }

    void PointCloud::startCellAssignment()
    {
        for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
            if (!m_NormalsCalculated) {
                PixIter(cam_index)
                {
                    calculateNormals(i, cam_index);
                }
            }

            m_State.setState(PointCloudStreamState::CELLS);

            if (!m_CellsAssigned) {
                PixIter(cam_index)
                {
                    auto p = m_Points[cam_index][i].getPoint();

                    std::string key = Cell::getKey(m_BoundingBoxes[cam_index], m_CellSizes[cam_index], p);

                    if (!m_pCellByKey[cam_index].contains(key)) {
                        glm::vec3 color{ m_ColorDistribution->operator()(m_Generator),
                                         m_ColorDistribution->operator()(m_Generator),
                                         m_ColorDistribution->operator()(m_Generator) };
                        m_pCellByKey[cam_index].insert(std::make_pair(key, new Cell(key, &m_PlanarThreshold)));
                        m_ColorBypCell[cam_index].insert(std::make_pair(m_pCellByKey[cam_index][key], color));
                    }
                    m_pCellByKey[cam_index][key]->addPoint(&m_Points[cam_index][i]);
                }

                m_CellsAssigned = true;
            }
        }
    }

    void PointCloud::assignCells(int i, int cam_index)
    {
        auto p = m_Points[cam_index][i].getPoint();

        std::string key = Cell::getKey(m_BoundingBoxes[cam_index], m_CellSizes[cam_index], p);

        glm::vec3 col;

        if (m_ShowAverageNormals) {
            auto normal = m_pCellByKey[cam_index][key]->getNormalisedNormal();
            col = (normal + glm::vec3(1.0f)) / glm::vec3(2.0f);
        }
        else {
            col = m_ColorBypCell[cam_index][m_pCellByKey[cam_index][key]];
        }
        m_Points[cam_index][i].Vert.Color = { col.x,
                                              col.y,
                                              col.z,};
    }

    void PointCloud::startCellCalculation()
    {
        if (!m_CellsAssigned) {
            startCellAssignment();
            // PixIter assignCells(i);
        }
        m_State.setState(PointCloudStreamState::CALC_CELLS);

        for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
            if (m_PlanarpCells.empty()) {
                for (auto const& [_, cell] : m_pCellByKey[cam_index]) {
                    cell->calculateNDT();

                    if (cell->getType() == Cell::NDT_TYPE::Planar)
                        m_PlanarpCells[cam_index].push_back(cell);
                    else
                        m_NonPlanarpCells[cam_index].push_back(cell);
                }
                m_PointDistribution = std::make_unique<std::uniform_int_distribution<int>>(0, m_PlanarpCells.size());
            }
        }
    }

    void PointCloud::calculateCells(int i, int cam_index)
    {
        auto p = m_Points[cam_index][i].getPoint();

        std::string key = Cell::getKey(m_BoundingBoxes[cam_index], m_CellSizes[cam_index], p);

        glm::vec3 col{};

        auto cell = m_pCellByKey[cam_index][key];
        auto type = cell->getType();

        if (m_ShowAverageNormals) {
            auto normal = cell->getNormalisedNormal();
            col = (normal + glm::vec3(1.0f)) / glm::vec3(2.0f);
        }
        else {
            if (type == Cell::NDT_TYPE::Planar)
                col = glm::vec3{ 0.0f, 0.0f, 1.0f };
            else if (type == Cell::NDT_TYPE::Linear)
                col = glm::vec3{ 1.0f, 0.0f, 0.0f };
            else if (type == Cell::NDT_TYPE::Spherical)
                col = glm::vec3{ 0.0f, 1.0f, 0.0f };
        }

        m_Points[cam_index][i].Vert.Color = { col.x,
                                              col.y,
                                              col.z};

    }

    void PointCloud::doPlaneSegmentation()
    {
        std::vector<int> plane_points{};

        plane_points.push_back(m_PointDistribution->operator()(m_Generator));
    }

    void PointCloud::manipulateTranslation()
    {
        for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
            if (ImGui::CollapsingHeader(("Translation " + m_DepthCameras[cam_index]->getCameraName()).c_str()))
            {
                ImGui::SliderFloat3(("Rotation " + m_DepthCameras[cam_index]->getCameraName()).c_str(), &m_Rotation[cam_index].x, -1.0f, 1.0f);
                ImGui::SliderFloat3(("Translation " + m_DepthCameras[cam_index]->getCameraName()).c_str(), &m_Translation[cam_index].x, -2.0f, 2.0f);
            }
        }

        if (ImGui::CollapsingHeader("Scale"))
        {
            ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 10.0f);
        }
    }
}