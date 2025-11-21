#include <glad/glad.h>
#include "shader.h"

pwg::Shader::Shader(const std::string vertexFilePath, const std::string fragmentFilePath)
{
    // Read vertexFile and fragmentFile and store the strings
    std::string vertexCode = ReadFromShaderFile(vertexFilePath);
    std::string fragmentCode = ReadFromShaderFile(fragmentFilePath);

    m_shaderType = STANDARD;
    
    // Convert the shader source strings into character arrays
    const char* vertexSource = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();

    //Create vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    //Attach vertex shader code to the shader object and compile shader
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);


    //Check if compilation was successful
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        PWG_ERROR("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n {0}", infoLog);
    }

    //Create fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    //Attach fragment shader code to the shader object and compile shader
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);


    //Check if compilation was successful
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        PWG_ERROR("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n {0}", infoLog);
    }

    //Create shader program
    m_shaderID = glCreateProgram();

    //Attach shaders to the program and link them
    glAttachShader(m_shaderID, vertexShader);
    glAttachShader(m_shaderID, fragmentShader);
    glLinkProgram(m_shaderID);


    //Check if linking was successful
    glGetProgramiv(m_shaderID, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(m_shaderID, 512, NULL, infoLog);
        PWG_ERROR("ERROR::SHADER::PROGRAM::LINKING_FAILED\n {0}", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

pwg::Shader::Shader(const std::string computeFilePath)
{
    // Read vertexFile and fragmentFile and store the strings
    std::string computeCode = ReadFromShaderFile(computeFilePath); 
    std::string noiseCode = ReadFromShaderFile("../assets/shaders/FastNoiseLite.glsl");

    m_shaderType = COMPUTE;

    computeCode = noiseCode + "\n" + computeCode;

    // Convert the shader source strings into character arrays
    const char* computeSource = computeCode.c_str();

    //Create vertex shader
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);

    //Attach vertex shader code to the shader object and compile shader
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

    glDeleteShader(computeShader);
}

pwg::Shader::~Shader()
{
}

std::string pwg::Shader::ReadFromShaderFile(const std::string shaderPath)
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

void pwg::Shader::ActivateShader()
{
    glUseProgram(m_shaderID);
}

void pwg::Shader::DeleteShader()
{
    glDeleteProgram(m_shaderID);
}

void pwg::Shader::SetUniformBool(const std::string& name, bool value) const
{
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(name);
    glUniform1i(location, (int)value);
}

void pwg::Shader::SetUniformInt(const std::string& name, int value) const
{
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(name);
    glUniform1i(location, value);
}

void pwg::Shader::SetUniformFloat(const std::string& name, float value) const
{
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(name);
    glUniform1f(location, value);
}

void pwg::Shader::SetUniformMat4(const std::string& name, glm::mat4 value) const
{
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(name);
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void pwg::Shader::SetUniformVec2(const std::string& name, glm::vec2 value) const
{
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(name);
    glUniform2fv(location, 1, glm::value_ptr(value));
}

void pwg::Shader::SetUniformVec3(const std::string& name, glm::vec3 value) const
{
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(name);
    glUniform3fv(location, 1, glm::value_ptr(value));
}

int pwg::Shader::GetUniformLocation(const std::string& uniformName)
{
    if (m_uniformLocations.contains(uniformName))
    {
        return m_uniformLocations.at(uniformName);
    }

    int location = glGetUniformLocation(m_shaderID, uniformName.c_str());
    m_uniformLocations[uniformName] = location;
    return location;
}

int pwg::Shader::GetUniformInt(const std::string& uniformName) const
{
    int value;
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(uniformName);
    glGetUniformiv(m_shaderID, location, &value);
    return value;
}

float pwg::Shader::GetUniformFloat(const std::string& uniformName) const
{
    float value;
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(uniformName);
    glGetUniformfv(m_shaderID, location, &value);
    return value;
}

glm::mat4 pwg::Shader::GetUniformMat4(const std::string& uniformName) const
{
    glm::mat4 value(0);
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(uniformName);
    glGetUniformfv(m_shaderID, location, glm::value_ptr(value));
    return value;
}

glm::vec2 pwg::Shader::GetUniformVec2(const std::string& uniformName) const
{
    glm::vec2 value(0);
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(uniformName);
    glGetUniformfv(m_shaderID, location, glm::value_ptr(value));
    return value;
}

glm::vec3 pwg::Shader::GetUniformVec3(const std::string& uniformName) const
{
    glm::vec3 value(0);
    int location = const_cast<pwg::Shader*>(this)->GetUniformLocation(uniformName);
    glGetUniformfv(m_shaderID, location, glm::value_ptr(value));
    return value;
}

unsigned int pwg::Shader::GetShaderID()
{
    return m_shaderID;
}
