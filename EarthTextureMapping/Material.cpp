#include "Material.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>


Material::Material(const std::string& filePath)
{
	ShaderProgramSource shaderSources = ParseShaderFile(filePath);
	m_MaterialID = CreateShaderProgram(shaderSources.vertexShader, shaderSources.fragmentShader);
}

Material::~Material()
{
	glDeleteProgram(m_MaterialID);
}

// Given file contains all shader code parses 
// and returns seperate shader files
ShaderProgramSource Material::ParseShaderFile(const std::string& filePath)
{
	std::ifstream file(filePath);
	std::stringstream ss[2];

	enum class ShaderType
	{
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	ShaderType type = ShaderType::NONE;

	std::string line;
	while (std::getline(file, line))
	{
		if (line.find("#shader") != std::string::npos)
		{
			if (line.find("vertex") != std::string::npos)
				type = ShaderType::VERTEX;
			else if (line.find("fragment") != std::string::npos)
				type = ShaderType::FRAGMENT;
		}
		else
			ss[static_cast<int>(type)] << line << "\n";
	}

	return { ss[0].str(), ss[1].str() };
}

// Compile the shader with given type and source file
unsigned int Material::CompileShader(unsigned int type, const std::string& shaderSource)
{
	unsigned int id = glCreateShader(type); // Get a shader id for a type
	const char* source = shaderSource.c_str(); // Convert source to type of const char*
	glShaderSource(id, 1, &source, nullptr); // Attain source file to the shader

	// Compile and check the status 
	glCompileShader(id); // Compile Shader
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char *errMessage = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, errMessage);

		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!\n";
		std::cout << errMessage << "\n";
		glDeleteShader(id);
		return 0;
	}
	// -- 

	return id;
}

// Create a program and attach shaders to it
unsigned int Material::CreateShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource)
{
	unsigned int program = glCreateProgram(); // Get ID for a program

	// Compile shaders
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
	// -- 

	// Attach shaders to the program
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	// --

	int result;
	// Link and check status
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (result == GL_FALSE)
	{
		int length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char *errMessage = (char*)alloca(length * sizeof(char));
		glGetProgramInfoLog(program, length, &length, errMessage);
		std::cout << "Error linking program: " << errMessage << "\n";
	}
	// --

	// Validate and check status
	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &result);
	if (result == GL_FALSE)
	{
		int length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char *errMessage = (char*)alloca(length * sizeof(char));
		glGetProgramInfoLog(program, length, &length, errMessage);
		std::cout << "Error validating program: " << errMessage << "\n";
	}
	// --

	// Delete shaders
	glDeleteShader(vs);
	glDeleteShader(fs);
	// --

	return program;
}

void Material::Bind() const 
{ 
	glUseProgram(m_MaterialID); 
}

void Material::Unbind() const 
{ 
	glUseProgram(0);
}