#include <glad/glad.h>
#include "computeShader.h"

namespace pwg
{
	ComputeShader::ComputeShader(const std::string& computeFilePath)
	{
        // Read compute file and store the strings
        std::string computeCode = ReadFromShaderFile(computeFilePath);

        // Convert the shader source strings into character arrays
        const char* computeSource = computeCode.c_str();

        //Create compute shader
        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);

        //Attach compute shader code to the shader object and compile shader
        glShaderSource(computeShader, 1, &computeSource, NULL);
        glCompileShader(computeShader);


        //Check if compilation was successful
        int success;
        char infoLog[512];
        glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
            PWG_ERROR("ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n {0}", infoLog);
        }

        //Create shader program
        m_shaderID = glCreateProgram();

        //Attach shaders to the program and link them
        glAttachShader(m_shaderID, computeShader);
        glLinkProgram(m_shaderID);


        //Check if linking was successful
        glGetProgramiv(m_shaderID, GL_LINK_STATUS, &success);

        if (!success)
        {
            glGetProgramInfoLog(m_shaderID, 512, NULL, infoLog);
            PWG_ERROR("ERROR::SHADER::PROGRAM::LINKING_FAILED\n {0}", infoLog);
        }

        int workGroupSize[3];
        glGetProgramiv(m_shaderID, GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize);

        m_localGroupSize.x = workGroupSize[0];
        m_localGroupSize.y = workGroupSize[1];
        m_localGroupSize.z = workGroupSize[2];

        glDeleteShader(computeShader);
	}

	ComputeShader::ComputeShader(const std::string& computeFilePath, const std::string& shaderToAttachFilePath)
	{
        // Read compute file and store the strings
        std::string computeCode = ReadFromShaderFile(computeFilePath);
        std::string include = ReadFromShaderFile(shaderToAttachFilePath);

        computeCode = include + "\n" + computeCode;

        // Convert the shader source strings into character arrays
        const char* computeSource = computeCode.c_str();

        //Create compute shader
        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);

        //Attach compute shader code to the shader object and compile shader
        glShaderSource(computeShader, 1, &computeSource, NULL);
        glCompileShader(computeShader);


        //Check if compilation was successful
        int success;
        char infoLog[512];
        glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
            PWG_ERROR("ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n {0}", infoLog);
        }

        //Create shader program
        m_shaderID = glCreateProgram();

        //Attach shaders to the program and link them
        glAttachShader(m_shaderID, computeShader);
        glLinkProgram(m_shaderID);


        //Check if linking was successful
        glGetProgramiv(m_shaderID, GL_LINK_STATUS, &success);

        if (!success)
        {
            glGetProgramInfoLog(m_shaderID, 512, NULL, infoLog);
            PWG_ERROR("ERROR::SHADER::PROGRAM::LINKING_FAILED\n {0}", infoLog);
        }

        int workGroupSize[3];
        glGetProgramiv(m_shaderID, GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize);

        m_localGroupSize.x = workGroupSize[0];
        m_localGroupSize.y = workGroupSize[1];
        m_localGroupSize.z = workGroupSize[2];

        glDeleteShader(computeShader);
	}

	ComputeShader::~ComputeShader()
	{
	}

	std::string ComputeShader::ReadFromShaderFile(const std::string& shaderPath)
	{
		std::string content{ "" };
		std::ifstream fileStream(shaderPath, std::ios::in);

		if (fileStream.is_open()) {
			std::string line = "";
			while (!fileStream.eof()) {
				std::getline(fileStream, line);
				content.append(line + "\n");
			}
		}
		else
		{
			PWG_ERROR("Could not read shader file ({0})", shaderPath);
		}

		fileStream.close();
		return content;
	}

	void ComputeShader::ActivateShader()
	{
        glUseProgram(m_shaderID);
	}

	void ComputeShader::DeleteShader()
	{
        glDeleteProgram(m_shaderID);
	}

	void ComputeShader::Dispatch(const int x, const int y, const int z)
	{
        glDispatchCompute(x, y, z);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
	}

    void ComputeShader::DispatchForTexture(const int width, const int height)
    {
        int x = (width + m_localGroupSize.x - 1) / m_localGroupSize.x;
        int y = (height + m_localGroupSize.y - 1) / m_localGroupSize.y;
        Dispatch(x, y);
    }

    void ComputeShader::MemoryBarrier(const int bits)
    {
        glMemoryBarrier(bits);
    }

    void ComputeShader::BindImage(const unsigned int textureUnit, const unsigned int textureID, const unsigned int access, const unsigned int format)
    {
        glBindImageTexture(textureUnit, textureID, 0, GL_FALSE, 0, access, format);
    }

    void ComputeShader::BindTextureSampler(const unsigned int textureUnit, const unsigned int textureID)
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    void ComputeShader::SetUniformBool(const std::string& name, bool value) const
    {
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(name);
        glUniform1i(location, (int)value);
    }

    void ComputeShader::SetUniformInt(const std::string& name, int value) const
    {
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(name);
        glUniform1i(location, value);
    }

    void ComputeShader::SetUniformFloat(const std::string& name, float value) const
    {
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(name);
        glUniform1f(location, value);
    }

    void ComputeShader::SetUniformMat4(const std::string& name, glm::mat4 value) const
    {
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(name);
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

    void ComputeShader::SetUniformVec2(const std::string& name, glm::vec2 value) const
    {
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(name);
        glUniform2fv(location, 1, glm::value_ptr(value));
    }

    void ComputeShader::SetUniformVec3(const std::string& name, glm::vec3 value) const
    {
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(name);
        glUniform3fv(location, 1, glm::value_ptr(value));
    }

    int ComputeShader::GetUniformLocation(const std::string& uniformName)
    {
        if (m_uniformLocations.contains(uniformName))
        {
            return m_uniformLocations.at(uniformName);
        }

        int location = glGetUniformLocation(m_shaderID, uniformName.c_str());
        m_uniformLocations[uniformName] = location;
        return location;
    }

    int ComputeShader::GetUniformInt(const std::string& uniformName) const
    {
        int value;
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(uniformName);
        glGetUniformiv(m_shaderID, location, &value);
        return value;
    }

    float ComputeShader::GetUniformFloat(const std::string& uniformName) const
    {
        float value;
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(uniformName);
        glGetUniformfv(m_shaderID, location, &value);
        return value;
    }

    glm::mat4 ComputeShader::GetUniformMat4(const std::string& uniformName) const
    {
        glm::mat4 value(0);
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(uniformName);
        glGetUniformfv(m_shaderID, location, glm::value_ptr(value));
        return value;
    }

    glm::vec2 ComputeShader::GetUniformVec2(const std::string& uniformName) const
    {
        glm::vec2 value(0);
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(uniformName);
        glGetUniformfv(m_shaderID, location, glm::value_ptr(value));
        return value;
    }

    glm::vec3 ComputeShader::GetUniformVec3(const std::string& uniformName) const
    {
        glm::vec3 value(0);
        int location = const_cast<ComputeShader*>(this)->GetUniformLocation(uniformName);
        glGetUniformfv(m_shaderID, location, glm::value_ptr(value));
        return value;
    }

    unsigned int ComputeShader::GetShaderID()
    {
        return m_shaderID;
    }
    
}
