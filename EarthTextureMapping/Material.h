#ifndef _MATERIAL_
#define _MATERIAL_

#include <string>
#include <unordered_map>
#include <iostream>

#include <GL/glew.h>

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

class Texture;
// Stores source file for all sub-shader types
struct ShaderProgramSource
{
	std::string vertexShader;
	std::string fragmentShader;
};

class Material
{
private:
	unsigned int m_MaterialID;
public:
	Material(const std::string& filePath);
	~Material();

	void Bind() const;
	void Unbind() const;

	unsigned int GetProgramID() { return m_MaterialID; }
private:
	// --- Shader Initializaiton ---
	ShaderProgramSource ParseShaderFile(const std::string& filePath);
	unsigned int CompileShader(unsigned int type, const std::string& shaderSource);
	unsigned int CreateShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource);
	// -----------------------------
};

#endif