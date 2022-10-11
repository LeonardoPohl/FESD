#include "PointCloud.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

namespace GLObject
{
    PointCloud::PointCloud(DepthCamera *depth_camera)
        : m_DepthCamera(depth_camera)
    {
        const int width = m_DepthCamera->getDepthStreamWidth();
        const int height = m_DepthCamera->getDepthStreamHeight();

        const size_t numElements = width * height;
        const size_t numIndex = numElements * Point::IndexCount;

        m_Points = new Point[numElements];
        unsigned int *indices = new unsigned int[numIndex];

        for (unsigned int h = 0; h < height; h++)
        {
            for (unsigned int w = 0; w < width; w++)
            {
                int i = h * width + w;
                m_Points[i].Position = { (float)h, (float)w };
                
                memcpy(indices + i * Point::IndexCount, m_Points[i].getIndices(i), Point::IndexCount * sizeof(unsigned int));
                m_Points[i].setVertexArray();
            }
        }

        // Indices for vertices order

        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_VAO = std::make_unique<VertexArray>();

        m_VB = std::make_unique<VertexBuffer>(width * height * Point::VertexCount * sizeof(Point::Vertex));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<GLfloat>(3);
        m_VBL->Push<GLfloat>(3);

        m_VAO->AddBuffer(*m_VB, *m_VBL);

        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, numIndex);

        m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_Shader->Bind();

        m_Vertices = new Point::Vertex[width * height * Point::VertexCount] {};

        //m_Proj = { glm::perspective(glm::radians(60.0f), (float)960.0f / 540.0f, 0.1f, 100.0f) };
        m_Proj = glm::ortho(0, width, 0, height, 0, 6000);
    }

    void PointCloud::OnRender()
    {
        if (!m_RenderPointCloud)
        {
            return;
        }
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto depth = m_DepthCamera->getDepth();

        const unsigned int width = m_DepthCamera->getDepthStreamWidth();
        const unsigned int height = m_DepthCamera->getDepthStreamHeight();

        for (unsigned int h = 0; h < height; h++)
        {
            for (unsigned int w = 0; w < width; w++)
            {
                int i = h * width + w;
                // Read depth data
                m_Points[i].updateDepth((float)(((uint8_t *)depth)[i]));
                m_Points[i].Position = { (float)h, (float)w };
                // Copy vertices into vertex array
                memcpy(m_Vertices + i * Point::VertexCount, &m_Points[i].Vertices[0], Point::VertexCount * sizeof(Point::Vertex));
            }
        }

        m_IndexBuffer->Bind();
        std::cout << sizeof(m_Vertices) * width * height * Point::VertexCount << std::endl;
        
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(m_Vertices) * width * height * Point::VertexCount, m_Vertices));

        Renderer renderer;

        glm::mat4 model = glm::mat4(1.0f);        

        // Assigns different transformations to each matrix
        model = glm::translate(glm::mat4(1.0f), m_Translation) * glm::rotate(model, glm::radians(m_RotationFactor), m_Rotation);

        glm::mat4 mvp = m_Proj * m_View * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4f("u_MVP", mvp);
        m_Shader->SetUniform1f("u_scale", m_Scale);
        renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
    }

    void PointCloud::OnImGuiRender()
    {
        ImGui::SliderFloat("Rotation Factor", &m_RotationFactor, 0.0f, 360.0f);
        ImGui::SliderFloat3("Rotation", &m_Rotation.x, 0.0f, 1.0f);
        ImGui::SliderFloat3("Translation", &m_Translation.x, -1.0f, 1.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.0f, 10.0f);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
}