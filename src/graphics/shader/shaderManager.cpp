#include "shaderManager.h"

void pwg::ShaderManager::Load(const std::string& shaderID, const std::string& vertexPath, const std::string& fragmentPath)
{
	if (m_shaders.contains(shaderID))
	{
		return;
	}

	auto shader = std::make_shared<Shader>(vertexPath, fragmentPath);
	m_shaders[shaderID] = shader;
	PWG_DEBUG("Shader has been added to map ({0}, {1}, {2})", shaderID, vertexPath, fragmentPath);
	
}

void pwg::ShaderManager::LoadCompute(const std::string& shaderID, const std::string& computePath)
{
	if (m_shaders.contains(shaderID))
	{
		return;
	}

	auto shader = std::make_shared<ComputeShader>(computePath);
	m_shaders[shaderID] = shader;
	PWG_DEBUG("Compute shader has been added to map ({0}, {1})", shaderID, computePath);
}

void pwg::ShaderManager::LoadComputeWithInclude(const std::string& shaderID, const std::string& computePath, const std::string& includePath)
{
	if (m_shaders.contains(shaderID))
	{
		return;
	}

	auto shader = std::make_shared<ComputeShader>(computePath, includePath);
	m_shaders[shaderID] = shader;
	PWG_DEBUG("Compute shader with include has been added to map ({0}, {1}, {2})", shaderID, computePath, includePath);
}

void pwg::ShaderManager::Unload(std::string& shaderID)
{
	m_shaders.erase(shaderID);
	PWG_DEBUG("Shader has been unloaded ({0})", shaderID);
}

void pwg::ShaderManager::UnloadAll()
{
	m_shaders.clear();
	PWG_DEBUG("All shaders have been unloaded");
}

