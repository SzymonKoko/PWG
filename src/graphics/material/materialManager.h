#ifndef SRC_GRAPHICS_MATERIAL_MATERIAL_MANAGER_H_
#define SRC_GRAPHICS_MATERIAL_MATERIAL_MANAGER_H_

#include "material.h"

namespace pwg
{
	/**
	* @brief Manages creation, storage, and lifetime of materials.
	* Provides centralized access to all materials used by the renderer.
	*/
	class MaterialManager
	{
	public:

		/**
		* @brief Default constructor for the material manager.
		*/
		MaterialManager() = default;

		/**
		* @brief Default destructor for the material manager.
		*/
		~MaterialManager() = default;

		/**
		* @brief Creates a new material and registers it under a given name.
		* @param name Unique identifier for the material.
		* @param shader Shader used by the material.
		* @return Shared pointer to the created material.
		*/
		std::shared_ptr<Material> CreateMaterial(const std::string& name, std::shared_ptr<Shader> shader);

		/**
		* @brief Retrieves a material by name.
		* @param name Material identifier.
		* @return Shared pointer to the material, or nullptr if not found.
		*/
		std::shared_ptr<Material> GetMaterial(const std::string& name);

		/**
		* @brief Returns a reference to the container holding all materials.
		* @return Unordered map of material names to material instances.
		*/
		std::unordered_map<std::string, std::shared_ptr<Material>>& GetAllMaterials();

		/**
		* @brief Destroys and removes a material from the manager.
		* @param name Identifier of the material to remove.
		*/
		void DestroyMaterial(const std::string& name);

	private:
		std::unordered_map<std::string, std::shared_ptr<Material>> m_materials; /**< Stored materials mapped by name */
	};
}

#endif // !SRC_GRAPHICS_MATERIAL_MATERIAL_MANAGER_H_
