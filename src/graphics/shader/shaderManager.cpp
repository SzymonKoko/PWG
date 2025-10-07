#include "shaderManager.h"

void pwg::ShaderManager::Load(const std::string& shaderID, const std::string& vertexPath, const std::string& fragmentPath)
{
	if (m_shaders.contains(shaderID))
	{
		return;
	}

	
	auto shader = std::make_shared<Shader>(vertexPath, fragmentPath);
	m_shaders[shaderID] = shader;
	std::cout << "Shader " << shaderID << " added to map!\n";
	
}

void pwg::ShaderManager::Unload(std::string& shaderID)
{
	m_shaders.erase(shaderID);
}

void pwg::ShaderManager::UnloadAll()
{
	m_shaders.clear();
}

std::shared_ptr<pwg::Shader> pwg::ShaderManager::GetShader(const std::string& shaderID)
{
	if (!m_shaders.contains(shaderID))
	{
		std::cout << "Shader does not exits!\n";
		return nullptr;
	}
	std::cout << "Found shader " << shaderID << std::endl;
	return m_shaders.at(shaderID);
}
