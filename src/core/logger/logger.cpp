#include "logger.h"

pwg::Logger::Logger()
{

}


void pwg::Logger::Log(const Module module, const Severity logLevel, const std::string& message)
{
	std::cout << "[" << SeverityToString(logLevel) << "] " << "[" << ModuleToString(module) << "] " << message << std::endl;
}


void pwg::Logger::LogInfo(const Module module, const std::string& message)
{
	Logger::Log(module, Severity::INFO, message);
}


void pwg::Logger::LogDebug(const Module module, const std::string& message)
{
	Logger::Log(module, Severity::DEBUG, message);
}


void pwg::Logger::LogError(const Module module, const std::string& message)
{
	Logger::Log(module, Severity::ERROR, message);
}


std::string pwg::Logger::ModuleToString(const Module module)
{
    using M = pwg::Logger::Module;
    switch (module)
    {
    case M::Application:    return "Application";
    case M::Window:         return "Window";
    case M::IndexBuffer:    return "IndexBuffer";
    case M::VertexArray:    return "VertexArray";
    case M::VertexBuffer:   return "VertexBuffer";
    case M::IMesh:          return "IMesh";
    case M::TriangleMesh:   return "TriangleMesh";
    case M::Renderer:       return "Renderer";
    case M::Shader:         return "Shader";
    case M::Logger:         return "Logger";
    case M::PyramidMesh:    return "PyramidMesh";
    case M::KeyboardInput:  return "KeyboardInput";
    case M::MouseInput:     return "MouseInput";
    case M::InputManager:   return "InputManager";
    case M::Camera:         return "Camera";
    case M::Texture:        return "Texture";
    case M::WindowEditor:   return "WindowEditor";
    case M::TerrainEditor:  return "TerrainEditor";
    case M::GUI:            return "GUI";
    case M::Noise:          return "Noise";
    default:                return "Unknown";
    }
}

std::string pwg::Logger::SeverityToString(const Severity logLevel)
{
    using S = pwg::Logger::Severity;
    switch (logLevel)
    {
    case S::INFO:           return "INFO";
    case S::DEBUG:          return "DEBUG";
    case S::ERROR:          return "ERROR";
    default:                return "Unknown";
    }
}

