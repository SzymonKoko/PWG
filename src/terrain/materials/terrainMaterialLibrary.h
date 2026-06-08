#ifndef SRC_TERRAIN_MATERIALS_TERRAIN_MATERIAL_LIBRARY_H_
#define SRC_TERRAIN_MATERIALS_TERRAIN_MATERIAL_LIBRARY_H_

#include "graphics/material/material.h"
#include "resources/resourceManager.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace pwg
{
	enum class TerrainMaterialRole
	{
		Grass = 0,
		Rock = 1,
		Sediment = 2,
		Snow = 3
	};

	constexpr int TERRAIN_MATERIAL_SLOT_COUNT = 4;

	struct TerrainMaterialDefinition
	{
		std::string name;
		TerrainMaterialRole role{ TerrainMaterialRole::Grass };
		std::string folder;
		std::string albedo;
		std::string normal;
		std::string roughness;
		std::string ao;
		std::string height;
		float tiling{ 1.0f };
		float normalStrength{ 1.0f };
		float roughnessScale{ 1.0f };
		float heightStrength{ 1.0f };
	};

	struct TerrainMaterialValidation
	{
		bool valid{ true };
		std::string message{ "Ready" };
		int width{ 0 };
		int height{ 0 };
	};

	class TerrainMaterialLibrary
	{
	public:
		TerrainMaterialLibrary(std::shared_ptr<ResourceManager> resourceManager, std::shared_ptr<Material> terrainMaterial);

		void ResetToDefaults();
		bool ReloadTerrainMaterials();
		bool LoadPbrFolder(TerrainMaterialRole role, const std::string& folder);
		bool ShowGui();
		void ApplyMaterialSettings();

		std::array<TerrainMaterialDefinition, TERRAIN_MATERIAL_SLOT_COUNT>& GetMaterials();
		const std::array<TerrainMaterialDefinition, TERRAIN_MATERIAL_SLOT_COUNT>& GetMaterials() const;
		void SetMaterials(const std::array<TerrainMaterialDefinition, TERRAIN_MATERIAL_SLOT_COUNT>& materials);
		TerrainMaterialValidation Validate() const;
		const std::string& GetStatus() const;

	private:
		struct MaterialPreview
		{
			std::shared_ptr<Texture> albedo;
			std::shared_ptr<Texture> normal;
			std::shared_ptr<Texture> roughness;
			std::shared_ptr<Texture> ao;
			std::shared_ptr<Texture> height;
		};

		static const char* RoleName(TerrainMaterialRole role);
		static int RoleIndex(TerrainMaterialRole role);
		static std::string FindTextureByToken(const std::string& folder, const std::vector<std::string>& tokens);
		static bool ReadImageSize(const std::string& path, int& width, int& height);
		static std::string PickFolderDialog();

		void RefreshPreviews();
		void RefreshPreview(TerrainMaterialRole role);
		void DrawMaterialSlot(TerrainMaterialRole role, bool& changed);
		std::vector<std::string> BuildAlbedoPaths() const;
		std::vector<std::string> BuildNormalPaths() const;
		std::vector<std::string> BuildDataPaths() const;

		std::shared_ptr<ResourceManager> m_resourceManager;
		std::shared_ptr<Material> m_terrainMaterial;
		std::array<TerrainMaterialDefinition, TERRAIN_MATERIAL_SLOT_COUNT> m_materials;
		std::array<MaterialPreview, TERRAIN_MATERIAL_SLOT_COUNT> m_previews;
		std::string m_status{ "Ready" };
	};
}

#endif // !SRC_TERRAIN_MATERIALS_TERRAIN_MATERIAL_LIBRARY_H_
