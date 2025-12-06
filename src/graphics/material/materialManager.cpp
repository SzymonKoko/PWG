#include "materialManager.h"

namespace pwg
{
	std::shared_ptr<Material> MaterialManager::CreateMaterial(const std::string& name, Shader& shader)
	{
		auto material = std::make_shared<Material>(shader);
		m_materials[name] = material;
		return material;
	}

	std::shared_ptr<Material> MaterialManager::GetMaterial(const std::string& name)
	{
		return m_materials[name];
	}

	std::unordered_map<std::string, std::shared_ptr<Material>>& MaterialManager::GetAllMaterials()
	{
		return m_materials;
	}

	void MaterialManager::DestroyMaterial(const std::string& name)
	{
		m_materials.erase(name);
	}
}


