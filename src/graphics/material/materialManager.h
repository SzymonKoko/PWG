#ifndef SRC_GRAPHICS_MATERIAL_MATERIAL_MANAGER_H_
#define SRC_GRAPHICS_MATERIAL_MATERIAL_MANAGER_H_

#include "material.h"

namespace pwg
{
	class MaterialManager
	{
	public:
		
		MaterialManager() = default;
		~MaterialManager() = default;

		std::shared_ptr<Material> CreateMaterial(const std::string& name, Shader& shader);
		std::shared_ptr<Material> GetMaterial(const std::string& name);
		std::unordered_map<std::string, std::shared_ptr<Material>>& GetAllMaterials();
		void DestroyMaterial(const std::string& name);

	private:
		std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;
	};
}

#endif // !SRC_GRAPHICS_MATERIAL_MATERIAL_MANAGER_H_
