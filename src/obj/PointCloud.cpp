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
    PointCloud::PointCloud(std::vector<DepthCamera*> depthCameras, const Camera* cam, Logger::Logger* logger, Renderer* renderer) : m_DepthCameras(depthCameras), m_CameraCount(depthCameras.size()), mp_Logger(logger)
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
            if (m_State == m_State.AVERAGING) {
                depth = static_cast<const int16_t*>(cam->getDepth());
                if (depth != nullptr) {
                    streamDepth(cam_index, depth);
                }
            }
            else if (m_State == m_State.NORMALS) {
                calculateNormals(cam_index);
            }
            else if (m_State == m_State.ICP) {
                alignPointclouds();
            }

            GLCall(glBufferSubData(GL_ARRAY_BUFFER, 
                                    sizeof(Point) * m_ElementOffset[cam_index], 
                                    sizeof(Point) * m_NumElements[cam_index], 
                                    m_Points[cam_index].get()));
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

        if (m_State != m_State.NORMALS && ImGui::Button("Show Normals")) {
            for (int cam_index = 0; cam_index < m_CameraCount; cam_index++) {
                calculateNormals(cam_index);
            }
        }

        if (ImGui::Button("(Re)Start Averaging")) {
            m_State.m_State = m_State.AVERAGING;
            m_AveragingCount = 0;
        }

        if (m_State == m_State.AVERAGING) {
            if (m_AveragingCount >= 200)
                m_AveragingCount = 200;
            else 
                m_AveragingCount += 1;
            
            if (ImGui::Button("Stop Averaging")) {
                m_AveragingCount = 0;
                m_State.m_State = m_State.STREAM;
            }
        }

        ImGui::InputFloat("Transformation Epsilon", &m_TransformationEpsilon, 0.01);
        ImGui::InputFloat("Step Size", &m_StepSize, 0.05);
        ImGui::InputFloat("Resolution", &m_Resolution, 0.1);
        ImGui::InputInt("Max Iterations", &m_MaxIterations);

        if (ImGui::Button("(Re)Align Pointclouds")) {
            m_IsAligned = false;
            alignPointcloudsNDT();//alignPointclouds();
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
        m_NormalsCalculated = false;
    }

    void PointCloud::streamDepth(int cam_index, const int16_t *depth)
    {
        PixIter(cam_index)
        {
            // Rotate the stream
            int depth_i = m_StreamWidths[cam_index] * (m_StreamHeights[cam_index] + 1) - i;
            m_BoundingBoxes[cam_index].updateBox(m_Points[cam_index][i].getPoint());
            auto adapted_depth = (float)depth[depth_i] * m_DepthCameras[cam_index]->getMetersPerUnit();
            
            if (m_NumElements[cam_index] - i < m_StreamWidths[cam_index]) {
                adapted_depth = 0.0f;
            }

            if (m_State.m_State == m_State.AVERAGING) {
                m_Points[cam_index][i].updateVertexArray(adapted_depth, cam_index, m_AveragingCount);
            }
            else {
                m_Points[cam_index][i].updateVertexArray(adapted_depth, cam_index);
            }
        }
    }

    void PointCloud::calculateNormals(int cam_index)
    {
        m_State.setState(PointCloudStreamState::NORMALS);
        m_NormalsCalculated = true;

        PixIter(cam_index) {
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
    }

    void PointCloud::alignPointcloudsNDT() {
        m_CloudIn = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[0], 1);

        for (int i = 0; i < m_NumElements[0]; i++) {
            auto p = &(m_CloudIn.get()->at(i));
            auto point = m_Points[0][i].getPoint();
            p->x = point.x;
            p->y = point.y;
            p->z = point.z;
        }

        std::cout << "Loaded " << m_CloudIn->size() << " data points from room_scan1.pcd" << std::endl;
        
        // Loading second scan of room from new perspective.
        m_CloudOut = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[1], 1);

        for (int i = 0; i < m_NumElements[1]; i++) {
            if (m_Points[1][i].Depth == 0.0f)
                continue;
            auto p = &(m_CloudOut.get()->at(i));
            auto point = m_Points[1][i].getPoint();
            p->x = point.x;
            p->y = point.y;
            p->z = point.z;
        }

        std::cout << "Loaded " << m_CloudOut->size() << " data points from room_scan2.pcd" << std::endl;
        
        // Filtering input scan to roughly 10% of original size to increase speed of registration.
        pcl::PointCloud<pcl::PointXYZ>::Ptr filtered_cloud(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::ApproximateVoxelGrid<pcl::PointXYZ> approximate_voxel_filter;
        approximate_voxel_filter.setLeafSize(0.2, 0.2, 0.2);
        approximate_voxel_filter.setInputCloud(m_CloudOut);
        approximate_voxel_filter.filter(*filtered_cloud);
        std::cout << "Filtered cloud contains " << filtered_cloud->size()
        << " data points from room_scan2.pcd" << std::endl;
        
        // Initializing Normal Distributions Transform (NDT).
        pcl::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ> ndt;

        // Setting scale dependent NDT parameters
        // Setting minimum transformation difference for termination condition.
        ndt.setTransformationEpsilon(m_TransformationEpsilon);
        // Setting maximum step size for More-Thuente line search.
        ndt.setStepSize(m_StepSize);
        //Setting Resolution of NDT grid structure (VoxelGridCovariance).
        ndt.setResolution(m_Resolution);
        
        // Setting max number of registration iterations.
        ndt.setMaximumIterations(m_MaxIterations);
        
        // Setting point cloud to be aligned.
        ndt.setInputSource(filtered_cloud);
        // Setting point cloud to be aligned to.
        ndt.setInputTarget(m_CloudIn);

        glm::mat4 model{ 1.0f };

        model = glm::eulerAngleYXZ(m_Rotation.y, m_Rotation.x, m_Rotation.z);
        model = glm::translate(model, m_Translation);

        Eigen::Matrix4f eigen_model;

        for (int x = 0; x < 4; x++) {
            for (int y = 0; y < 4; y++) {
                eigen_model(x, y) = model[x][y];
            }
        }
        // Set initial alignment estimate found using robot odometry.
        //Eigen::AngleAxisf init_rotation(0.6931, Eigen::Vector3f::UnitZ());
        //Eigen::Translation3f init_translation(1.79387, 0.720047, 0);
        //Eigen::Matrix4f init_guess = (init_translation * init_rotation).matrix();
        
        // Calculating required rigid transform to align the input cloud to the target cloud.
        pcl::PointCloud<pcl::PointXYZ>::Ptr output_cloud(new pcl::PointCloud<pcl::PointXYZ>);
        ndt.align(*output_cloud, eigen_model);
        
        mp_Logger->log("Normal Distributions Transform has converged: " + std::to_string(ndt.hasConverged()) + " score: " + std::to_string(ndt.getFitnessScore()));
        
        // Transforming unfiltered, input cloud using found transform.
        //pcl::transformPointCloud(*m_CloudOut, *output_cloud, ndt.getFinalTransformation());
        
        // Saving transformed input cloud.
        //pcl::io::savePCDFileASCII("room_scan2_transformed.pcd", *output_cloud);

        auto transform = Eigen::Affine3f(ndt.getFinalTransformation());
        pcl::getTranslationAndEulerAngles(transform,
            m_Translation.x, m_Translation.y, m_Translation.z,
            m_Rotation.x, m_Rotation.y, m_Rotation.z);
        /*
        // Initializing point cloud visualizer
        pcl::visualization::PCLVisualizer::Ptr
        viewer_final(new pcl::visualization::PCLVisualizer("3D Viewer"));
        viewer_final->setBackgroundColor(0, 0, 0);
        
        // Coloring and visualizing target cloud (red).
        pcl::visualization::PointCloudColorHandlerCustom < pcl::PointXYZ>
        target_color(target_cloud, 255, 0, 0);
        viewer_final->addPointCloud<pcl::PointXYZ>(target_cloud, target_color, "target cloud");
        viewer_final->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                                        1, "target cloud");
        
        // Coloring and visualizing transformed input cloud (green).
        pcl::visualization::PointCloudColorHandlerCustom < pcl::PointXYZ>
        output_color(output_cloud, 0, 255, 0);
        viewer_final->addPointCloud<pcl::PointXYZ>(output_cloud, output_color, "output cloud");
        viewer_final->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE,
                                                          1, "output cloud");
        
         // Starting visualizer
         viewer_final->addCoordinateSystem(1.0, "global");
         viewer_final->initCameraParameters();
        
         // Wait until visualizer window is closed.
         while (!viewer_final->wasStopped())
         {
           viewer_final->spinOnce(100);
           std::this_thread::sleep_for(100ms);
         }*/
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
            m_AlignmentMode = true;
        }

        if (m_IsAligned) {
            // Set the max correspondence distance to 5cm (e.g., correspondences with higher
            // distances will be ignored)
            m_ICP.setMaxCorrespondenceDistance(0.5);
            // Set the maximum number of iterations (criterion 1)
            m_ICP.setMaximumIterations(100);
            // Set the transformation epsilon (criterion 2)
            m_ICP.setTransformationEpsilon(1e-8);
            // Set the euclidean distance difference epsilon (criterion 3)
            m_ICP.setEuclideanFitnessEpsilon(1);

            m_CloudOut = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[1], 1);

            glm::mat4 model{ 1.0f };

            model = glm::eulerAngleYXZ(m_Rotation.y, m_Rotation.x, m_Rotation.z);
            model = glm::translate(model, m_Translation);

            Eigen::Matrix4f eigen_model;

            for (int x = 0; x < 4; x++) {
                for (int y = 0; y < 4; y++) {
                    eigen_model(x, y) = model[x][y];
                }
            }

            for (int i = 0; i < m_NumElements[1]; i++) {
                if (m_Points[1][i].Depth == 0.0f)
                    continue;
                auto p = &(m_CloudOut.get()->at(i));
                auto point = m_Points[1][i].getPoint();
                p->x = point.x;
                p->y = point.y;
                p->z = point.z;
            }
            pcl::transformPointCloud(*m_CloudOut.get(), *m_CloudOut.get(), eigen_model);
            m_ICP.setInputTarget(m_CloudOut);

            pcl::PointCloud<pcl::PointXYZ> final;
            m_ICP.align(final);

            auto transform = Eigen::Affine3f(m_ICP.getFinalTransformation());
            pcl::getTranslationAndEulerAngles(transform,
                                              m_Translation.x, m_Translation.y, m_Translation.z,
                                              m_Rotation.x, m_Rotation.y, m_Rotation.z );
        }   
    }
}