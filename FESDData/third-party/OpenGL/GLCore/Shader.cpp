#include "Shader.h"

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <filesystem>

#include <GL/glew.h>

#include "GLErrorManager.h"

Shader::Shader(const std::string &filepath)
	: m_RendererID(0)
{
    std::filesystem::path fp{filepath};
    ShaderProgramSource source;
    if (fp.has_extension() && fp.extension() == ".shader") {
        source = ParseShaderCombined(filepath);
    }
    else {
        for (const auto& entry : std::filesystem::directory_iterator(fp))
        {
            if (entry.is_regular_file()) {
                std::ifstream file(entry.path(), std::ios::in | std::ios::binary);
                if (!file.is_open())
                    continue;

                if (entry.path().extension() == ".vert") {
                    source.VertexSource = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
                }
                else if (entry.path().extension() == ".frag") {
                    source.FragmentSource = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
                }

                file.close();
            }
        }
    }
    m_RendererID = CreateShader(source.VertexSource, source.FragmentSource);
}

Shader::~Shader()
{
    GLCall(glDeleteProgram(m_RendererID));
}

ShaderProgramSource Shader::ParseShaderCombined(const std::string& filepath)
{
    std::ifstream stream(filepath);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
            {
                type = ShaderType::VERTEX;
            }
            else if (line.find("fragment") != std::string::npos)
            {
                type = ShaderType::FRAGMENT;
            }
        }
        else
        {
            ss[(int)type] << line << "\n";
        }
    }
    return{ ss[0].str(), ss[1].str() };
}

unsigned int Shader::CompileShader(unsigned int type, const std::string &source)
{
    unsigned int id = glCreateShader(type);
    const char *src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if ( result == GL_FALSE )
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char *message = (char *)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);

        std::cout << "Failed to compile " << ( type == GL_VERTEX_SHADER ? "vertex" : "fragment" ) << " shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}


unsigned int Shader::CreateShader(const std::string &vertexShader, const std::string &fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void Shader::Bind() const
{
    GLCall(glUseProgram(m_RendererID));
}

void Shader::Unbind() const
{
    GLCall(glUseProgram(0));
}

void Shader::SetUniformBool(const std::string& name, bool value)
{
    GLCall(glUniform1i(GetUniformLocation(name), value));
}

void Shader::SetUniform1i(const std::string &name, int value)
{
    GLCall(glUniform1i(GetUniformLocation(name), value));
}

void Shader::SetUniform1f(const std::string &name, float value)
{
    GLCall(glUniform1f(GetUniformLocation(name), value));
}

void Shader::SetUniform4f(const std::string &name, float v0, float v1, float v2, float v3)
{
    GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}

void Shader::SetUniformMat3f(const std::string &name, const glm::mat3 &matrix)
{
    GLCall(glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

void Shader::SetUniformMat4f(const std::string& name, const glm::mat4& matrix)
{
    GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

void Shader::SetUniformMat4fv(const std::string& name, const std::vector<glm::mat4>& matrices, size_t elemCount)
{
    GLCall(glUniformMatrix4fv(GetUniformLocation(name), elemCount, GL_FALSE, &matrices[0][0][0]));
}

int Shader::GetUniformLocation(const std::string &name) const
{
    if ( m_UniformLocationCache.find(name) != m_UniformLocationCache.end() )
        return m_UniformLocationCache[name];
    
    GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));

    if ( location == -1 )
        std::cout << "[WARNING] Uniform '" << name << "' not found!" << std::endl;

    m_UniformLocationCache[name] = location;

    return location;
}