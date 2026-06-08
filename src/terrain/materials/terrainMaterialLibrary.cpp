#include "terrainMaterialLibrary.h"

#include "imgui.h"
#include "platform/nativeDialogs.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stb/stb_image.h>

namespace pwg
{
	namespace
	{
		std::string NormalizePath(const std::string& path)
		{
			return std::filesystem::path(path).generic_string();
		}

		bool ContainsToken(const std::string& value, const std::string& token)
		{
			std::string lowerValue = value;
			std::string lowerToken = token;
			std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			std::transform(lowerToken.begin(), lowerToken.end(), lowerToken.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return lowerValue.find(lowerToken) != std::string::npos;
		}
	}

	TerrainMaterialLibrary::TerrainMaterialLibrary(std::shared_ptr<ResourceManager> resourceManager, std::shared_ptr<Material> terrainMaterial)
		: m_resourceManager(resourceManager),
		  m_terrainMaterial(terrainMaterial)
	{
		ResetToDefaults();
	}

	void TerrainMaterialLibrary::ResetToDefaults()
	{
		m_materials[0] = {
			"Grass002_2K-JPG",
			TerrainMaterialRole::Grass,
			"../assets/textures/pbr/pbr_grass2",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_Color.jpg",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_NormalGL.jpg",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_Roughness.jpg",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_AmbientOcclusion.jpg",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_Displacement.jpg"
		};
		m_materials[1] = {
			"Rock060_2K-JPG",
			TerrainMaterialRole::Rock,
			"../assets/textures/pbr/pbr_stone2",
			"../assets/textures/pbr/pbr_stone2/Rock060_2K-JPG_Color.jpg",
			"../assets/textures/pbr/pbr_stone2/Rock060_2K-JPG_NormalGL.jpg",
			"../assets/textures/pbr/pbr_stone2/Rock060_2K-JPG_Roughness.jpg",
			"../assets/textures/pbr/pbr_stone2/Rock060_2K-JPG_AmbientOcclusion.jpg",
			"../assets/textures/pbr/pbr_stone2/Rock060_2K-JPG_Displacement.jpg"
		};
		m_materials[2] = {
			"Ground079S_2K-JPG",
			TerrainMaterialRole::Sediment,
			"../assets/textures/pbr/pbr_sediment",
			"../assets/textures/pbr/pbr_sediment/Ground079S_2K-JPG_Color.jpg",
			"../assets/textures/pbr/pbr_sediment/Ground079S_2K-JPG_NormalGL.jpg",
			"../assets/textures/pbr/pbr_sediment/Ground079S_2K-JPG_Roughness.jpg",
			"../assets/textures/pbr/pbr_sediment/Ground079S_2K-JPG_AmbientOcclusion.jpg",
			"../assets/textures/pbr/pbr_sediment/Ground079S_2K-JPG_Displacement.jpg"
		};
		m_materials[3] = {
			"Snow Placeholder",
			TerrainMaterialRole::Snow,
			"../assets/textures/pbr/pbr_grass2",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_Color.jpg",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_NormalGL.jpg",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_Roughness.jpg",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_AmbientOcclusion.jpg",
			"../assets/textures/pbr/pbr_grass2/Grass002_2K-JPG_Displacement.jpg",
			1.35f,
			0.35f,
			1.15f,
			0.35f
		};

		RefreshPreviews();
		ReloadTerrainMaterials();
	}

	bool TerrainMaterialLibrary::ReloadTerrainMaterials()
	{
		TerrainMaterialValidation validation = Validate();
		m_status = validation.message;
		if (!validation.valid)
		{
			return false;
		}

		auto textureManager = m_resourceManager->GetTextureManager();
		textureManager->ReloadTextureArray("terrainTexturesAlbedo", BuildAlbedoPaths(), TextureType::ALBEDO);
		textureManager->ReloadTextureArray("terrainTexturesNormal", BuildNormalPaths(), TextureType::NORMAL);
		textureManager->ReloadTextureArray("terrainTexturesData", BuildDataPaths(), TextureType::DATA);

		m_terrainMaterial->SetTextureArray("u_TexturesAlbedo", textureManager->GetTextureArray("terrainTexturesAlbedo"));
		m_terrainMaterial->SetTextureArray("u_TexturesNormal", textureManager->GetTextureArray("terrainTexturesNormal"));
		m_terrainMaterial->SetTextureArray("u_TexturesData", textureManager->GetTextureArray("terrainTexturesData"));

		ApplyMaterialSettings();

		m_status = "Terrain materials reloaded";
		return true;
	}

	bool TerrainMaterialLibrary::LoadPbrFolder(TerrainMaterialRole role, const std::string& folder)
	{
		if (folder.empty() || !std::filesystem::is_directory(folder))
		{
			m_status = "Invalid material folder";
			return false;
		}

		TerrainMaterialDefinition& material = m_materials[RoleIndex(role)];
		material.folder = NormalizePath(folder);
		material.name = std::filesystem::path(folder).filename().string();
		material.albedo = FindTextureByToken(folder, { "Color", "Albedo", "BaseColor" });
		material.normal = FindTextureByToken(folder, { "NormalGL", "Normal" });
		material.roughness = FindTextureByToken(folder, { "Roughness" });
		material.ao = FindTextureByToken(folder, { "AmbientOcclusion", "AO" });
		material.height = FindTextureByToken(folder, { "Displacement", "Height" });

		if (material.ao.empty())
		{
			material.ao = material.roughness;
		}
		if (material.height.empty())
		{
			material.height = material.roughness;
		}

		RefreshPreview(role);
		m_status = "Loaded folder for " + std::string(RoleName(role));
		return true;
	}

	bool TerrainMaterialLibrary::ShowGui()
	{
		bool changed = false;

		TerrainMaterialValidation validation = Validate();
		ImGui::Text("Status: %s", validation.message.c_str());
		if (validation.width > 0 && validation.height > 0)
		{
			ImGui::Text("Texture size: %d x %d", validation.width, validation.height);
		}

		if (ImGui::Button("Reload Terrain Materials"))
		{
			changed = ReloadTerrainMaterials();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset To Defaults"))
		{
			ResetToDefaults();
			changed = true;
		}

		ImGui::Separator();
		DrawMaterialSlot(TerrainMaterialRole::Grass, changed);
		ImGui::Separator();
		DrawMaterialSlot(TerrainMaterialRole::Rock, changed);
		ImGui::Separator();
		DrawMaterialSlot(TerrainMaterialRole::Sediment, changed);
		ImGui::Separator();
		DrawMaterialSlot(TerrainMaterialRole::Snow, changed);
		if (changed)
		{
			ApplyMaterialSettings();
		}
		return changed;
	}

	void TerrainMaterialLibrary::ApplyMaterialSettings()
	{
		for (int i = 0; i < TERRAIN_MATERIAL_SLOT_COUNT; ++i)
		{
			m_terrainMaterial->SetUniformFloat("u_materialTiling[" + std::to_string(i) + "]", m_materials[i].tiling);
			m_terrainMaterial->SetUniformFloat("u_materialNormalStrength[" + std::to_string(i) + "]", m_materials[i].normalStrength);
			m_terrainMaterial->SetUniformFloat("u_materialRoughnessScale[" + std::to_string(i) + "]", m_materials[i].roughnessScale);
			m_terrainMaterial->SetUniformFloat("u_materialHeightStrength[" + std::to_string(i) + "]", m_materials[i].heightStrength);
		}
	}

	std::array<TerrainMaterialDefinition, TERRAIN_MATERIAL_SLOT_COUNT>& TerrainMaterialLibrary::GetMaterials()
	{
		return m_materials;
	}

	const std::array<TerrainMaterialDefinition, TERRAIN_MATERIAL_SLOT_COUNT>& TerrainMaterialLibrary::GetMaterials() const
	{
		return m_materials;
	}

	void TerrainMaterialLibrary::SetMaterials(const std::array<TerrainMaterialDefinition, TERRAIN_MATERIAL_SLOT_COUNT>& materials)
	{
		m_materials = materials;
		RefreshPreviews();
	}

	TerrainMaterialValidation TerrainMaterialLibrary::Validate() const
	{
		TerrainMaterialValidation result;
		int expectedWidth = 0;
		int expectedHeight = 0;

		for (const auto& material : m_materials)
		{
			std::vector<std::pair<std::string, std::string>> required = {
				{ "albedo", material.albedo },
				{ "normal", material.normal },
				{ "roughness", material.roughness }
			};
			for (const auto& [label, path] : required)
			{
				if (path.empty() || !std::filesystem::exists(path))
				{
					result.valid = false;
					result.message = material.name + " missing " + label + " texture";
					return result;
				}
			}

			std::vector<std::string> paths = { material.albedo, material.normal, material.roughness, material.ao, material.height };
			for (const auto& path : paths)
			{
				int width = 0;
				int height = 0;
				if (!ReadImageSize(path, width, height))
				{
					result.valid = false;
					result.message = "Cannot read texture: " + path;
					return result;
				}
				if (expectedWidth == 0)
				{
					expectedWidth = width;
					expectedHeight = height;
				}
				else if (width != expectedWidth || height != expectedHeight)
				{
					result.valid = false;
					result.message = "Texture size mismatch. Expected " + std::to_string(expectedWidth) + "x" + std::to_string(expectedHeight) + ", got " + std::to_string(width) + "x" + std::to_string(height);
					return result;
				}
			}
		}

		result.width = expectedWidth;
		result.height = expectedHeight;
		result.message = "Ready";
		return result;
	}

	const std::string& TerrainMaterialLibrary::GetStatus() const
	{
		return m_status;
	}

	const char* TerrainMaterialLibrary::RoleName(TerrainMaterialRole role)
	{
		switch (role)
		{
		case TerrainMaterialRole::Grass:
			return "Grass";
		case TerrainMaterialRole::Rock:
			return "Rock";
		case TerrainMaterialRole::Sediment:
			return "Sediment";
		case TerrainMaterialRole::Snow:
			return "Snow";
		default:
			return "Unknown";
		}
	}

	int TerrainMaterialLibrary::RoleIndex(TerrainMaterialRole role)
	{
		return static_cast<int>(role);
	}

	std::string TerrainMaterialLibrary::FindTextureByToken(const std::string& folder, const std::vector<std::string>& tokens)
	{
		std::vector<std::filesystem::path> files;
		for (const auto& entry : std::filesystem::directory_iterator(folder))
		{
			if (!entry.is_regular_file())
			{
				continue;
			}
			std::string extension = entry.path().extension().string();
			if (extension != ".jpg" && extension != ".png" && extension != ".jpeg" && extension != ".tga")
			{
				continue;
			}
			files.push_back(entry.path());
		}

		for (const auto& token : tokens)
		{
			for (const auto& file : files)
			{
				std::string filename = file.filename().string();
				if (ContainsToken(filename, token))
				{
					return NormalizePath(file.string());
				}
			}
		}
		return {};
	}

	bool TerrainMaterialLibrary::ReadImageSize(const std::string& path, int& width, int& height)
	{
		int channels = 0;
		return stbi_info(path.c_str(), &width, &height, &channels) != 0;
	}

	std::string TerrainMaterialLibrary::PickFolderDialog()
	{
		return platform::PickFolderDialog("Select PBR material folder");
	}

	void TerrainMaterialLibrary::RefreshPreviews()
	{
		RefreshPreview(TerrainMaterialRole::Grass);
		RefreshPreview(TerrainMaterialRole::Rock);
		RefreshPreview(TerrainMaterialRole::Sediment);
	}

	void TerrainMaterialLibrary::RefreshPreview(TerrainMaterialRole role)
	{
		int index = RoleIndex(role);
		const auto& material = m_materials[index];
		auto load = [](const std::string& path, TextureType type) -> std::shared_ptr<Texture>
		{
			if (path.empty() || !std::filesystem::exists(path))
			{
				return nullptr;
			}
			return std::make_shared<Texture>(path, type);
		};
		m_previews[index].albedo = load(material.albedo, TextureType::ALBEDO);
		m_previews[index].normal = load(material.normal, TextureType::NORMAL);
		m_previews[index].roughness = load(material.roughness, TextureType::ALBEDO);
		m_previews[index].ao = load(material.ao, TextureType::ALBEDO);
		m_previews[index].height = load(material.height, TextureType::ALBEDO);
	}

	void TerrainMaterialLibrary::DrawMaterialSlot(TerrainMaterialRole role, bool& changed)
	{
		int index = RoleIndex(role);
		auto& material = m_materials[index];
		auto& preview = m_previews[index];

		ImGui::PushID(index);
		ImGui::Text("%s", RoleName(role));
		ImGui::SameLine();
		ImGui::TextDisabled("%s", material.name.c_str());

		if (ImGui::Button("Load PBR Folder..."))
		{
			std::string folder = PickFolderDialog();
			if (!folder.empty())
			{
				changed = LoadPbrFolder(role, folder) || changed;
			}
		}

		changed = ImGui::SliderFloat("Tiling", &material.tiling, 0.1f, 8.0f) || changed;
		changed = ImGui::SliderFloat("Normal", &material.normalStrength, 0.0f, 2.0f) || changed;
		changed = ImGui::SliderFloat("Roughness", &material.roughnessScale, 0.1f, 2.0f) || changed;
		changed = ImGui::SliderFloat("Height", &material.heightStrength, 0.0f, 2.0f) || changed;

		auto drawPreview = [](const char* label, const std::shared_ptr<Texture>& texture)
		{
			ImGui::BeginGroup();
			ImGui::Text("%s", label);
			if (texture)
			{
				ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture->GetTextureID())), ImVec2(72.0f, 72.0f), ImVec2(0, 1), ImVec2(1, 0));
			}
			else
			{
				ImGui::Dummy(ImVec2(72.0f, 72.0f));
			}
			ImGui::EndGroup();
		};

		drawPreview("Albedo", preview.albedo);
		ImGui::SameLine();
		drawPreview("Normal", preview.normal);
		ImGui::SameLine();
		drawPreview("Rough", preview.roughness);
		ImGui::SameLine();
		drawPreview("AO", preview.ao);
		ImGui::SameLine();
		drawPreview("Height", preview.height);

		ImGui::TextWrapped("Folder: %s", material.folder.c_str());
		ImGui::PopID();
	}

	std::vector<std::string> TerrainMaterialLibrary::BuildAlbedoPaths() const
	{
		return { m_materials[0].albedo, m_materials[1].albedo, m_materials[2].albedo, m_materials[3].albedo };
	}

	std::vector<std::string> TerrainMaterialLibrary::BuildNormalPaths() const
	{
		return { m_materials[0].normal, m_materials[1].normal, m_materials[2].normal, m_materials[3].normal };
	}

	std::vector<std::string> TerrainMaterialLibrary::BuildDataPaths() const
	{
		return {
			m_materials[0].roughness, m_materials[1].roughness, m_materials[2].roughness, m_materials[3].roughness,
			m_materials[0].roughness, m_materials[1].roughness, m_materials[2].roughness, m_materials[3].roughness,
			m_materials[0].ao, m_materials[1].ao, m_materials[2].ao, m_materials[3].ao,
			m_materials[0].height, m_materials[1].height, m_materials[2].height, m_materials[3].height
		};
	}
}
