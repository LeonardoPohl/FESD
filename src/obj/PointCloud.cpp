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

        if (ImGui::Button("Acquire Data")) {
            
        }

        if (ImGui::TreeNode("NDT")) {
            ImGui::InputFloat("Transformation Epsilon", &m_TransformationEpsilon, 0.01);
            ImGui::InputFloat("Step Size", &m_StepSize, 0.05);
            ImGui::InputFloat("Resolution", &m_Resolution, 0.1);
            ImGui::InputInt("Max Iterations", &m_MaxIterations);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Feature Based")) {
            ImGui::Checkbox("Use ISS", &m_UseISS);

            ImGui::Checkbox("Use SIFT", &m_UseSIFT);
            
            ImGui::BeginDisabled(!m_UseSIFT);
            ImGui::InputFloat("Min Scale", &m_MinScale);
            ImGui::InputInt("N Octaves", &m_NOctaves);
            ImGui::InputInt("N ScalesPerOctave", &m_NScalesPerOctave);
            ImGui::InputFloat("Min Contrast", &m_MinContrast);
            ImGui::EndDisabled();

            if (ImGui::Button("Gather Keypoints")) {
                acquireData();
                estimateKeyPoints();
            }
            ImGui::TreePop();
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
    glm::vec3 PointCloud::getRotation()
    {
        return m_Rotation;
    }

    glm::vec3 PointCloud::getTranslation()
    {
        return m_Translation;
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

            m_Points[cam_index][i].updateVertexArray(adapted_depth, cam_index);
            
        }
    }

    void PointCloud::acquireData()
    {
        if (m_State.m_State == m_State.REGISTRATION)
            return;

        m_State.m_State = m_State.REGISTRATION;
        m_CloudStatic = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[0], 1);

        for (int i = 0; i < m_NumElements[0]; i++) {
            if (m_Points[1][i].Depth == 0.0f)
                continue;
            auto p = &(m_CloudStatic.get()->at(i));
            auto point = m_Points[0][i].getPoint();
            p->x = point.x;
            p->y = point.y;
            p->z = point.z;
        }

        mp_Logger->log("Loaded static Point cloud with " + std::to_string(m_CloudStatic->size()) + " data points");

        // Loading second scan of room from new perspective.
        m_CloudDynamic = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[1], 1);

        for (int i = 0; i < m_NumElements[1]; i++) {
            if (m_Points[1][i].Depth == 0.0f)
                continue;
            auto p = &(m_CloudDynamic.get()->at(i));
            auto point = m_Points[1][i].getPoint();
            p->x = point.x;
            p->y = point.y;
            p->z = point.z;
        }
        mp_Logger->log("Loaded dynamic Point cloud with " + std::to_string(m_CloudDynamic->size()) + " data points");
    }

    void PointCloud::filterData() {
        // Filtering input scan to roughly 10% of original size to increase speed of registration.
        pcl::PointCloud<pcl::PointXYZ>::Ptr filtered_cloud_dynamic(new pcl::PointCloud<pcl::PointXYZ>);
        pcl::ApproximateVoxelGrid<pcl::PointXYZ> approximate_voxel_filter;
        approximate_voxel_filter.setLeafSize(0.2, 0.2, 0.2);
        approximate_voxel_filter.setInputCloud(m_CloudDynamic);
        approximate_voxel_filter.filter(*m_CloudDynamic);
        mp_Logger->log("Filtered dynamic cloud contains " + std::to_string(m_CloudDynamic->size()) + " data points");
    }

    double computeCloudResolution(const pcl::PointCloud<pcl::PointXYZ>::ConstPtr& cloud)
    {
        double res = 0.0;
        int n_points = 0;
        int nres;
        std::vector<int> indices(2);
        std::vector<float> sqr_distances(2);
        pcl::search::KdTree<pcl::PointXYZ> tree;
        tree.setInputCloud(cloud);

        for (std::size_t i = 0; i < cloud->size(); ++i)
        {
            if (!std::isfinite((*cloud)[i].x))
            {
                continue;
            }
            //Considering the second neighbor since the first is the point itself.
            nres = tree.nearestKSearch(i, 2, indices, sqr_distances);
            if (nres == 2)
            {
                res += sqrt(sqr_distances[1]);
                ++n_points;
            }
        }
        if (n_points != 0)
        {
            res /= n_points;
        }
        return res;
    }

    void PointCloud::estimateKeyPoints()
    {
        if (m_UseISS)
        {// ISS 3D
            m_ISSKPStatic = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
            m_ISSKPDynamic = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
            pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>());

            double model_resolution = computeCloudResolution(m_CloudStatic);
            // Compute model_resolution

            pcl::ISSKeypoint3D<pcl::PointXYZ, pcl::PointXYZ> iss_detector;

            iss_detector.setSearchMethod(tree);
            iss_detector.setSalientRadius(6 * model_resolution);
            iss_detector.setNonMaxRadius(4 * model_resolution);
            iss_detector.setThreshold21(0.975);
            iss_detector.setThreshold32(0.975);
            iss_detector.setMinNeighbors(5);
            iss_detector.setNumberOfThreads(4);
            iss_detector.setInputCloud(m_CloudStatic);
            iss_detector.compute(*m_ISSKPStatic);
            iss_detector.setInputCloud(m_CloudDynamic);
            iss_detector.compute(*m_ISSKPDynamic);
            mp_Logger->log("ISS found " + std::to_string(m_ISSKPStatic->size()) + " key points (static)");
            mp_Logger->log("ISS found " + std::to_string(m_ISSKPDynamic->size()) + " key points (dynamic)");
        }
        
        if (m_UseSIFT)
        {
            m_SIFTKPStatic = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
            m_SIFTKPDynamic = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
            pcl::PointCloud<pcl::PointWithScale> result;
            pcl::search::KdTree<pcl::PointNormal>::Ptr tree(new pcl::search::KdTree<pcl::PointNormal>());
            pcl::PointCloud<pcl::PointNormal>::Ptr normals_static(new pcl::PointCloud<pcl::PointNormal>);
            pcl::PointCloud<pcl::PointNormal>::Ptr normals_dynamic(new pcl::PointCloud<pcl::PointNormal>);

            for (std::size_t i = 0; i < normals_static->size(); ++i)
            {
                (*normals_static)[i].x = (*m_SIFTKPStatic)[i].x;
                (*normals_static)[i].y = (*m_SIFTKPStatic)[i].y;
                (*normals_static)[i].z = (*m_SIFTKPStatic)[i].z;
            }

            for (std::size_t i = 0; i < normals_dynamic->size(); ++i)
            {
                (*normals_dynamic)[i].x = (*m_SIFTKPDynamic)[i].x;
                (*normals_dynamic)[i].y = (*m_SIFTKPDynamic)[i].y;
                (*normals_dynamic)[i].z = (*m_SIFTKPDynamic)[i].z;
            }

            pcl::SIFTKeypoint<pcl::PointNormal, pcl::PointWithScale> sift;

            sift.setSearchMethod(tree);
            sift.setScales(m_MinScale, m_NOctaves, m_NScalesPerOctave);
            sift.setMinimumContrast(m_MinContrast);
            sift.setInputCloud(normals_static);
            sift.compute(result);
            // Copying the pointwithscale to pointxyz so as visualize the cloud
            copyPointCloud(result, *m_SIFTKPStatic);

            sift.setInputCloud(normals_dynamic);
            sift.compute(result);
            // Copying the pointwithscale to pointxyz so as visualize the cloud
            copyPointCloud(result, *m_SIFTKPDynamic);

            mp_Logger->log("SIFT found " + std::to_string(m_ISSKPStatic->size()) + " key points (static)");
            mp_Logger->log("SIFT found " + std::to_string(m_ISSKPDynamic->size()) + " key points (dynamic)");
        }
    }

    void PointCloud::describeKeyPoints(KPEstimator estimator)
    {
        {// FPFH
            // Calculate normals for both pointclouds
            pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
            pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>());
            ne.setSearchMethod(tree);

            // Output datasets
            pcl::PointCloud<pcl::Normal>::Ptr normals_dynamic(new pcl::PointCloud<pcl::Normal>());
            pcl::PointCloud<pcl::Normal>::Ptr normals_static(new pcl::PointCloud<pcl::Normal>());

            // Use all neighbors in a sphere of radius 3cm
            ne.setRadiusSearch(0.03);
            if (estimator == ISS)
                ne.setInputCloud(m_ISSKPDynamic);
            else if (estimator == SIFT)
                ne.setInputCloud(m_SIFTKPDynamic);
            ne.compute(*normals_dynamic);

            if (estimator == ISS)
                ne.setInputCloud(m_ISSKPStatic);
            else if (estimator == SIFT)
                ne.setInputCloud(m_SIFTKPStatic);
            ne.compute(*normals_static);

            pcl::FPFHEstimation<pcl::PointXYZ, pcl::Normal, pcl::FPFHSignature33> fpfh;

            fpfh.setSearchMethod(tree);

            // Output datasets
            m_FPFHsDynamic.insert({ estimator, { std::make_shared<pcl::PointCloud<pcl::FPFHSignature33>>() } });
            m_FPFHsStatic.insert({ estimator, { std::make_shared<pcl::PointCloud<pcl::FPFHSignature33>>() } });

            // Use all neighbors in a sphere of radius 5cm
            // IMPORTANT: the radius used here has to be larger than the radius used to estimate the surface normals!!!
            fpfh.setRadiusSearch(0.05);

            // Compute the features
            if (estimator == ISS)
                fpfh.setInputCloud(m_ISSKPStatic);
            else if (estimator == SIFT)
                fpfh.setInputCloud(m_SIFTKPStatic);
            fpfh.setInputNormals(normals_static);
            fpfh.compute(*m_FPFHsStatic.at(estimator));

            if (estimator == ISS)
                fpfh.setInputCloud(m_ISSKPDynamic);
            else if (estimator == SIFT)
                fpfh.setInputCloud(m_SIFTKPDynamic);
            fpfh.setInputNormals(normals_dynamic);
            fpfh.compute(*m_FPFHsDynamic.at(estimator));

            mp_Logger->log("FPFH found " + std::to_string(m_FPFHsStatic.at(estimator)->size()) + " descriptors (static)");
            mp_Logger->log("FPFH found " + std::to_string(m_FPFHsDynamic.at(estimator)->size()) + " descriptors (dynamic)");
        }
        {

        }
    }

    /*
    void PointCloud::alignPointcloudsNDT() {
       
        
        // Initializing Normal Distributions Transform (NDT).
        pcl::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ> ndt;

        // Setting scale dependent NDT parameters
        // Setting minimum transformation difference for termination condition.
        //ndt.setTransformationEpsilon(m_TransformationEpsilon);
        // Setting maximum step size for More-Thuente line search.
        //ndt.setStepSize(m_StepSize);
        //Setting Resolution of NDT grid structure (VoxelGridCovariance).
        //ndt.setResolution(m_Resolution);
        
        // Setting max number of registration iterations.
        //ndt.setMaximumIterations(m_MaxIterations);
        
        // Setting point cloud to be aligned.
        ndt.setInputSource(filtered_cloud_dynamic);
        // Setting point cloud to be aligned to.
        ndt.setInputTarget(m_CloudStatic);

        glm::mat4 model{ 1.0f };

        model = glm::eulerAngleYXZ(m_Rotation.y, m_Rotation.x, m_Rotation.z);
        model = glm::translate(model, m_Translation);

        Eigen::Matrix4f init_guess;

        for (int x = 0; x < 4; x++) {
            for (int y = 0; y < 4; y++) {
                init_guess(x, y) = model[x][y];
            }
        }
        
        // Calculating required rigid transform to align the input cloud to the target cloud.
        pcl::PointCloud<pcl::PointXYZ>::Ptr output_cloud(new pcl::PointCloud<pcl::PointXYZ>);
        ndt.align(*output_cloud, init_guess);
        
        mp_Logger->log("Normal Distributions Transform has converged: " + std::to_string(ndt.hasConverged()) + " score: " + std::to_string(ndt.getFitnessScore()) + " In " + std::to_string(ndt.getFinalNumIteration()) + "/" + std::to_string(m_MaxIterations) + " Iterations");
        
        auto transform = Eigen::Affine3f(ndt.getFinalTransformation());
        pcl::getTranslationAndEulerAngles(transform,
            m_Translation.x, m_Translation.y, m_Translation.z,
            m_Rotation.x, m_Rotation.y, m_Rotation.z);
    }

    void PointCloud::alignPointclouds()
    {
        if (!m_ICPInitialised) {
            m_State.setState(PointCloudStreamState::ICP);
            m_CloudStatic = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[0], 1);

            for (int i = 0; i < m_NumElements[0]; i++) {
                auto p = &(m_CloudStatic.get()->at(i));
                auto point = m_Points[0][i].getPoint();
                p->x = point.x;
                p->y = point.y;
                p->z = point.z;
            }

            m_ICP.setInputSource(m_CloudStatic);
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

            m_CloudDynamic = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>(m_NumElements[1], 1);

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
                auto p = &(m_CloudDynamic.get()->at(i));
                auto point = m_Points[1][i].getPoint();
                p->x = point.x;
                p->y = point.y;
                p->z = point.z;
            }
            pcl::transformPointCloud(*m_CloudDynamic.get(), *m_CloudDynamic.get(), eigen_model);
            m_ICP.setInputTarget(m_CloudDynamic);

            pcl::PointCloud<pcl::PointXYZ> final;
            m_ICP.align(final);

            auto transform = Eigen::Affine3f(m_ICP.getFinalTransformation());
            pcl::getTranslationAndEulerAngles(transform,
                                              m_Translation.x, m_Translation.y, m_Translation.z,
                                              m_Rotation.x, m_Rotation.y, m_Rotation.z );
        }   
    }*/
}