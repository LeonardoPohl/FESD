#include "PointCloud.h"

#include <iostream>
#include <ranges>
#include <random>
#include <execution>

#include <GLCore/GLErrorManager.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#define PixIter(cam_index) for(int i = 0; i < m_NumElements[cam_index]; i++)

namespace GLObject
{
    //
    // Constructor
    //
    PointCloud::PointCloud(std::vector<DepthCamera *> depthCameras, const Camera *cam, Renderer *renderer) : m_DepthCameras(depthCameras), m_CameraCount(depthCameras.size())
    {
        this->camera = cam;

        GLUtil::setFlags();

        for (auto cam : m_DepthCameras) {
            m_StreamWidths.push_back(cam->getDepthStreamWidth());
            m_StreamHeights.push_back(cam->getDepthStreamHeight());

            m_NumElements.push_back(m_StreamWidths.back() * m_StreamHeights.back());
            m_ElementOffset.push_back(m_NumElementsTotal);
            
            m_MVPS.push_back({ });

            m_Points.push_back(std::make_shared<Point[]>(m_NumElements.back()));

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

                    m_Points[cam_index][i].updateVertexArray(0.f, cam_index);
                    indices[i + m_ElementOffset[cam_index]] = i + m_ElementOffset[cam_index];
                }
            }
        }

        m_GLUtil.mp_Renderer = renderer;
        m_GLUtil.m_VAO = std::make_unique<VertexArray>(m_NumElementsTotal);

        m_GLUtil.m_VB = std::make_unique<VertexBuffer>(m_NumElementsTotal * sizeof(Point));
        m_GLUtil.m_VBL = std::make_unique<VertexBufferLayout>();

        // Position Function
        m_GLUtil.m_VBL->Push<GLfloat>(2);
        // Depth
        m_GLUtil.m_VBL->Push<GLfloat>(1);
        // Color
        m_GLUtil.m_VBL->Push<GLfloat>(3);
        // Camera Id
        m_GLUtil.m_VBL->Push<GLint>(1);
        
        m_GLUtil.m_VAO->AddBuffer(*m_GLUtil.m_VB, *m_GLUtil.m_VBL);

        m_GLUtil.m_IndexBuffer = std::make_unique<IndexBuffer>(indices, m_NumElementsTotal);

        m_GLUtil.m_Shader = std::make_unique<Shader>("resources/shaders/PointCloud");
        m_GLUtil.m_Shader->Bind();

        m_PointDistribution = std::make_unique<std::uniform_int_distribution<int>>(0, m_NumElementsTotal - 1);
        m_ColorDistribution = std::make_unique<std::uniform_int_distribution<int>>(0, 255);

        delete indices;
    }

    // 
    // Updates
    // 
    void PointCloud::OnUpdate()
    {
        const int16_t *depth;

        for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
            auto cam = m_DepthCameras[cam_index];
            if (!cam->m_IsEnabled)
                continue;

            if (m_State == m_State.STREAM) {
                depth = static_cast<const int16_t*>(cam->getDepth());
                if (depth != nullptr) {
        
                    streamDepth(cam_index, depth);
                }
            }
            else if (m_State == m_State.NORMALS) {
                startNormalCalculation();
                PixIter(cam_index)
                {
                    calculateNormals(i, cam_index);
                }
            }
            else if (m_State == m_State.ICP) {
                alignPointclouds();
            }
            auto p = m_Points[cam_index].get();
            GLCall(glBufferSubData(GL_ARRAY_BUFFER, sizeof(Point) * m_ElementOffset[cam_index], sizeof(Point) * m_NumElements[cam_index], m_Points[cam_index].get()));
        }
    }

    void PointCloud::OnRender()
    {
        m_GLUtil.m_Shader->Bind();

        glm::mat4 model{ 1.0f };

        model = glm::eulerAngleYXZ(m_Rotation.y, m_Rotation.x, m_Rotation.z); 
        model = glm::translate(model, m_Translation);

        m_GLUtil.m_Shader->SetUniformMat4f(("u_Model"), model);
        

        m_GLUtil.m_Shader->SetUniformMat4f("u_VP", camera->getViewProjection());
        m_GLUtil.m_Shader->SetUniformBool("u_AlignmentMode", m_AlignmentMode);
        m_GLUtil.mp_Renderer->DrawPoints(*m_GLUtil.m_VAO, *m_GLUtil.m_IndexBuffer, *m_GLUtil.m_Shader);
    }

    void PointCloud::OnImGuiRender()
    {
        if (m_State != m_State.STREAM && ImGui::Button("Resume Stream"))
            resumeStream();

        if (m_State == m_State.STREAM && ImGui::Button("Pause Stream"))
            pauseStream();

        if (m_State != m_State.NORMALS && ImGui::Button("Show Normals"))
            startNormalCalculation();

        if (ImGui::Button("Align Pointclouds")) {
            alignPointclouds();
        }

        ImGui::Checkbox("Alignment Mode", &m_AlignmentMode);

        manipulateTranslation();
    }
    
    void PointCloud::manipulateTranslation()
    {
        if (ImGui::CollapsingHeader("Translation"))
        {
            ImGui::InputFloat("Rotation x", &m_Rotation.x, 0.01f, 0.1f, "%.2f");
            ImGui::InputFloat("Rotation y", &m_Rotation.y, 0.01f, 0.1f, "%.2f");
            ImGui::InputFloat("Rotation z", &m_Rotation.z, 0.01f, 0.1f, "%.2f");

            ImGui::InputFloat("Translation x", &m_Translation.x, 0.001, 0.01, "%.3f");
            ImGui::InputFloat("Translation y", &m_Translation.y, 0.001, 0.01, "%.3f");
            ImGui::InputFloat("Translation z", &m_Translation.z, 0.001, 0.01, "%.3f");
        }

        if (ImGui::CollapsingHeader("Scale"))
        {
            ImGui::SliderFloat("Scale", &m_Scale, 0.001f, 10.0f);
        }
    }

    //
    // Getters
    //
    glm::vec3 PointCloud::getRotation(int cameraId)
    {
        if (cameraId == 0)
            return m_Rotation;
        return { };
    }

    glm::vec3 PointCloud::getTranslation(int cameraId)
    {
        if (cameraId == 0)
            return m_Translation;
        return { };
    }

    //
    // Pause and Resume
    //
    void PointCloud::pauseStream()
    {
        m_State.setState(PointCloudStreamState::IDLE);
    }

    void PointCloud::resumeStream()
    {
        m_State.setState(PointCloudStreamState::STREAM);
        
        m_ICPInitialised = false;
        m_ShowAverageNormals = false;
        m_NormalsCalculated = false;
    }

    void PointCloud::streamDepth(int cam_index, const int16_t *depth)
    {
        PixIter(cam_index)
        {
            // Rotate the stream
            int depth_i = m_StreamWidths[cam_index] * (m_StreamHeights[cam_index] + 1) - i;
            m_BoundingBoxes[cam_index].updateBox(m_Points[cam_index][i].getPoint());
            
            m_Points[cam_index][i].updateVertexArray((float)depth[depth_i] * m_DepthCameras[cam_index]->getMetersPerUnit(), cam_index);
        }
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

        m_Points[cam_index][i].Color = { (normal.x + 1) / 2.0f,
                                         (normal.y + 1) / 2.0f,
                                         (normal.z + 1) / 2.0f };
    }

    void PointCloud::alignPointclouds()
    {
        if (!m_ICPInitialised) {
            m_State.setState(PointCloudStreamState::ICP);
            m_CloudIn = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[0], 1);

            for (int i = 0; i < m_NumElements[0]; i++) {
                auto p = &(m_CloudIn.get()->at(i));
                auto point = m_Points[0][i].getPoint();
                p->x = point.x;
                p->y = point.y;
                p->z = point.z;
            }

            m_ICP.setInputSource(m_CloudIn);
            m_ICPInitialised = true;
        }

        if (!m_IsAligned) {
            m_CloudOut = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[1], 1);

            for (int i = 0; i < m_NumElements[1]; i++) {
                auto p = &(m_CloudOut.get()->at(i));
                auto point = m_Points[1][i].getPoint();
                p->x = point.x;
                p->y = point.y;
                p->z = point.z;
            }

            m_ICP.setInputTarget(m_CloudOut);

            pcl::PointCloud<pcl::PointXYZ> final;
            m_ICP.align(final);
            if (m_ICP.hasConverged()) {
                m_IsAligned = true;
            }
            auto transform = m_ICP.getFinalTransformation();
            std::cout << transform << std::endl;
        }
    }
}