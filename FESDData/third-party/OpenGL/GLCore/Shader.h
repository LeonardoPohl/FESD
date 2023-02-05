#pragma once
#include <array>
#include <string>
#include <unordered_map>

#include "glm/glm.hpp"

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragmentSource;
};

class Shader
{
private:
	std::string m_FilePath;
	unsigned int m_RendererID;
	// caching for uniforms
	mutable std::unordered_map<std::string, int> m_UniformLocationCache;
public:
	Shader() =default;
	Shader(const std::string& filepath);
	~Shader();

	void Bind() const;
	void Unbind() const;

	// Set uniforms

	void SetUniformBool(const std::string &name, bool value);
	void SetUniform1i(const std::string &name, int value);
	void SetUniform1f(const std::string &name, float value);
	void SetUniform4f(const std::string &name, float v0, float v1, float v2, float v3);
	void SetUniformMat3f(const std::string &name, const glm::mat3 &matrix);
	void SetUniformMat4f(const std::string &name, const glm::mat4 &matrix);
	void SetUniformMat4fv(const std::string& name, const std::vector<glm::mat4> &matrices, size_t elemCount);
private:
	unsigned int CreateShader(const std::string &vertexShader, const std::string &fragmentShader);
	ShaderProgramSource ParseShaderCombined(const std::string &filepath);
	unsigned int CompileShader(unsigned int type, const std::string &source);

	int GetUniformLocation(const std::string &name) const;
};