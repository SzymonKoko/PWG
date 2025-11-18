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

void pwg::ShaderManager::Load(const std::string& shaderID, const std::string& computePath)
{
	if (m_shaders.contains(shaderID))
	{
		return;
	}

	auto shader = std::make_shared<Shader>(computePath);
	m_shaders[shaderID] = shader;
	PWG_DEBUG("Compute shader has been added to map ({0}, {1})", shaderID, computePath);
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

std::shared_ptr<pwg::Shader> pwg::ShaderManager::GetShader(const std::string& shaderID)
{
	if (!m_shaders.contains(shaderID))
	{
		PWG_ERROR("Shader {0} does not exits!\n", shaderID);
		return nullptr;
	}
	return m_shaders.at(shaderID);
}
