#ifndef SHADER_H_
#define SHADER_H_

#include <glad.h>
#include <iostream>
#include <fstream>
#include <string>

namespace pwg
{
	class Shader
	{
	public:
		Shader(const char* vertexFilePath, const char* fragmentFilePath);
		~Shader();

		std::string ReadFromShaderFile(const char* shaderPath);
		void ActivateShader();
		void DeleteShader();
	private:
		GLuint m_shaderID{ 0 };
	};
}
#endif // !SHADER_H_
