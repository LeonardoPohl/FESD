#include "TestPyramid3D.h"

#include <GLCore/GLErrorManager.h>
#include <imgui.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace GLObject
{
    TestPyramid3D::TestPyramid3D(const Camera *cam)
    {
        this->camera = cam;
        GLfloat vertices[] =
        { //     COORDINATES     /        COLORS      /   TexCoord  //
            -0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	0.0f, 0.0f, // 0
            -0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	5.0f, 0.0f, // 1
             0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	0.0f, 0.0f, // 2
             0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	5.0f, 0.0f, // 3
             0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	2.5f, 5.0f  // 4
        };

        // Indices for vertices order
        GLuint indices[] =
        {
           1, 0, 2,
           2, 0, 3,
           1, 0, 4,
           2, 1, 4,
           3, 2, 4,
           0, 3, 4,
        };

        GLCall(glEnable(GL_BLEND));

        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        //GLCall(glEnable(GL_CULL_FACE));

        m_VAO = std::make_unique<VertexArray>();

        m_VB = std::make_unique<VertexBuffer>(vertices, 8 * 5 * sizeof(GLfloat));
        m_VBL = std::make_unique<VertexBufferLayout>();

        m_VBL->Push<GLfloat>(3);
        m_VBL->Push<GLfloat>(3);
        m_VBL->Push<GLfloat>(2);

        m_VAO->AddBuffer(*m_VB, *m_VBL);
        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, 6 * 3);

        m_Shader = std::make_unique<Shader>("resources/shaders/basic3d.shader");
        m_Shader->Bind();

        m_Texture = std::make_unique<Texture>("resources/textures/brick.png");
        m_Texture->Bind();
        m_Shader->SetUniform1i("u_Texture", 0);
        //m_Proj = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, -1.0f, 1.0f);
    }

    void TestPyramid3D::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer renderer;

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 proj = glm::mat4(1.0f);
        

        // Assigns different transformations to each matrix
        model = glm::translate(glm::mat4(1.0f), m_Translation) * glm::rotate(model, glm::radians(m_RotationFactor), m_Rotation);

        m_Shader->Bind();
        m_Shader->SetUniformMat4f("u_model", model);
        if (useCamera && camera)
        {
            m_Shader->SetUniformMat4f("u_view", camera->getView());
            m_Shader->SetUniformMat4f("u_proj", camera->getProjection());
        }
        else
        {
            m_Shader->SetUniformMat4f("u_view", m_View);
            m_Shader->SetUniformMat4f("u_proj", m_Proj);
        }
        m_Shader->SetUniform1f("u_scale", m_Scale);
        m_Texture->Bind();
        renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        
    }

    void TestPyramid3D::OnImGuiRender()
    {
        ImGui::SliderFloat("Rotation Factor", &m_RotationFactor, 0.0f, 360.0f);
        ImGui::SliderFloat3("Rotation", &m_Rotation.x, 0.0f, 1.0f);
        ImGui::SliderFloat3("Translation", &m_Translation.x, -5.0f, 5.0f);
        ImGui::SliderFloat("Scale", &m_Scale, 0.0f, 10.0f);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }
}