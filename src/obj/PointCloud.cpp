#include "PointCloud.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace GLObject
{
    PointCloud::PointCloud()
        : m_Color{ 0.2f, 0.3f, 0.8f, 1.0f }, m_RotationFactor(0), 
        m_Scale(0.5f), m_Rotation{ 0.0f, 1.0f, 0.0f }, m_Translation{ 0.0f, 1.0f, 0.0f },
        m_View(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -2.0f))),
        m_Proj(glm::perspective(glm::radians(45.0f), (float)960.0f / 540.0f, 0.1f, 100.0f))
    {
        // TODO: Allocate dynamically (camara resolution)

        int width = 640;
        int height = 480;

        // TODO: create all points

        // Indices for vertices order

        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_VAO = std::make_unique<VertexArray>();

        m_VB = std::make_unique<VertexBuffer>(width * height * 5 * sizeof(Point::Vertex));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<GLfloat>(3);
        m_VBL->Push<GLfloat>(4);

        m_VAO->AddBuffer(*m_VB, *m_VBL);

        // TODO: https://www.youtube.com/watch?v=v5UDqm3zvcw&list=PLlrATfBNZ98f5vZ8nJ6UengEkZUMC4fy5&index=5
        int indices[] =
        {
            0, 1, 2,
            0, 2, 3,
            0, 1, 4,
            1, 2, 4,
            2, 3, 4,
            3, 0, 4
        };

        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, 6 * 3);

        m_Shader = std::make_unique<Shader>("resources/shaders/pointcloud.shader");
        m_Shader->Bind();

        m_Texture = std::make_unique<Texture>("resources/textures/brick.png");
        m_Texture->Bind();
        m_Shader->SetUniform1i("u_Texture", 0);
        // TODO Generate Camera Class in OpenGL lib
    }

    void PointCloud::OnRender()
    {

        // TODO: Generate all pyramids:
        // auto pyramid0 = point0.getVertexArray(0);
        // auto pyramid0 = point1.getVertexArray(0);
        // ...
        // 
        // Point::Vertex vertecies[640 * 480 * 5];
        // 
        // memcpy(vertecies, pyramid0.data(), pyramid1.size() * sizeof(Point.Vertex));
        // memcpy(vertecies + pyramid0.size((, pyramid1.data(), pyramid1.size() * sizeof(Point.Vertex));
        // ....
        m_IndexBuffer->Bind();
        //GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);


        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer renderer;

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        

        // Assigns different transformations to each matrix
        model = glm::translate(glm::mat4(1.0f), m_Translation) * glm::rotate(model, glm::radians(m_RotationFactor), m_Rotation);

        glm::mat4 mvp = m_Proj * m_View * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4f("u_MVP", mvp);
        m_Shader->SetUniform1f("u_scale", m_Scale);
        m_Texture->Bind();
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