#include "shader.h"

pwg::Shader::Shader(const char* vertexFilePath, const char* fragmentFilePath)
{
    // Read vertexFile and fragmentFile and store the strings
    std::string vertexCode = ReadFromShaderFile(vertexFilePath);
    std::string fragmentCode = ReadFromShaderFile(fragmentFilePath);

    
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
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" <<
            infoLog << std::endl;
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
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" <<
            infoLog << std::endl;
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
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" <<
            infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    Logger::LogInfo(Logger::Module::Shader, "Shaders loaded");
	
}

pwg::Shader::~Shader()
{
}

std::string pwg::Shader::ReadFromShaderFile(const char* shaderPath)
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
        Logger::LogError(Logger::Module::Shader, "Could not read file" + (std::string)shaderPath);
        std::cout << "Could not read file " << shaderPath << ". File does not exist." << std::endl;
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

unsigned int pwg::Shader::GetShaderID()
{
    return m_shaderID;
}
