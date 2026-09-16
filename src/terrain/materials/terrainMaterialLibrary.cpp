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
		RefreshCatalog();
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
			"Snow014_2K-JPG",
			TerrainMaterialRole::Snow,
			"../assets/textures/pbr/Snow014_2K-JPG",
			"../assets/textures/pbr/Snow014_2K-JPG/Snow014_2K-JPG_Color.jpg",
			"../assets/textures/pbr/Snow014_2K-JPG/Snow014_2K-JPG_NormalGL.jpg",
			"../assets/textures/pbr/Snow014_2K-JPG/Snow014_2K-JPG_Roughness.jpg",
			"../assets/textures/pbr/Snow014_2K-JPG/Snow014_2K-JPG_AmbientOcclusion.jpg",
			"../assets/textures/pbr/Snow014_2K-JPG/Snow014_2K-JPG_Displacement.jpg",
			1.35f,
			0.35f,
			1.15f,
			0.35f
		};
		m_materials[3].minHeight = 0.70f;
		m_materials[3].blendSoftness = 0.10f;

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
		std::string error;
		auto albedo = textureManager->CreateTextureArray(BuildAlbedoPaths(), TextureType::ALBEDO, {}, &error);
		if (!albedo)
		{
			m_status = error;
			return false;
		}
		auto normal = textureManager->CreateTextureArray(BuildNormalPaths(), TextureType::NORMAL, {}, &error);
		if (!normal)
		{
			m_status = error;
			return false;
		}
		auto data = textureManager->CreateTextureArray(BuildDataPaths(), TextureType::DATA, BuildDataFallbackValues(), &error);
		if (!data)
		{
			m_status = error;
			return false;
		}

		textureManager->SetTextureArray("terrainTexturesAlbedo", albedo);
		textureManager->SetTextureArray("terrainTexturesNormal", normal);
		textureManager->SetTextureArray("terrainTexturesData", data);

		m_terrainMaterial->SetTextureArray("u_TexturesAlbedo", albedo);
		m_terrainMaterial->SetTextureArray("u_TexturesNormal", normal);
		m_terrainMaterial->SetTextureArray("u_TexturesData", data);

		ApplyMaterialSettings();

		m_status = "Terrain materials reloaded";
		return true;
	}

	bool TerrainMaterialLibrary::LoadPbrFolder(TerrainMaterialRole role, const std::string& folder)
	{
		PbrMaterialCatalogEntry entry = DiscoverCatalogEntry(folder);
		if (!entry.valid)
		{
			m_status = entry.validationMessage.empty() ? "Invalid material folder" : entry.validationMessage;
			return false;
		}
		return SelectCatalogMaterial(role, entry);
	}

	void TerrainMaterialLibrary::RefreshCatalog()
	{
		m_catalog.clear();
		m_catalogPreviews.clear();
		std::filesystem::path root = std::filesystem::path("../assets/textures/pbr");
		if (!std::filesystem::is_directory(root) && !m_materials[0].folder.empty())
		{
			root = std::filesystem::path(m_materials[0].folder).parent_path();
		}
		if (!std::filesystem::is_directory(root))
		{
			m_status = "PBR catalog folder not found";
			return;
		}
		std::vector<std::filesystem::path> folders;
		for (const auto& entry : std::filesystem::directory_iterator(root))
		{
			if (entry.is_directory()) folders.push_back(entry.path());
		}
		std::sort(folders.begin(), folders.end());
		for (const auto& folder : folders)
		{
			auto entry = DiscoverCatalogEntry(folder.string());
			m_catalog.push_back(entry);
			const std::string previewPath = !entry.preview.empty() ? entry.preview : entry.material.albedo;
			m_catalogPreviews.push_back(!previewPath.empty() && std::filesystem::exists(previewPath)
				? std::make_shared<Texture>(previewPath, TextureType::ALBEDO) : nullptr);
		}
		m_status = "Found " + std::to_string(m_catalog.size()) + " PBR materials";
	}

	const std::vector<PbrMaterialCatalogEntry>& TerrainMaterialLibrary::GetCatalog() const { return m_catalog; }

	bool TerrainMaterialLibrary::SelectCatalogMaterial(TerrainMaterialRole role, const PbrMaterialCatalogEntry& entry)
	{
		if (!entry.valid)
		{
			m_status = entry.validationMessage;
			return false;
		}
		const int index = RoleIndex(role);
		TerrainMaterialDefinition previous = m_materials[index];
		TerrainMaterialDefinition selected = entry.material;
		selected.role = role;
		selected.tiling = previous.tiling;
		selected.normalStrength = previous.normalStrength;
		selected.roughnessScale = previous.roughnessScale;
		selected.heightStrength = previous.heightStrength;
		selected.amount = previous.amount;
		selected.minHeight = previous.minHeight;
		selected.maxHeight = previous.maxHeight;
		selected.minSlope = previous.minSlope;
		selected.maxSlope = previous.maxSlope;
		selected.blendSoftness = previous.blendSoftness;
		m_materials[index] = selected;
		if (!ReloadTerrainMaterials())
		{
			m_materials[index] = previous;
			return false;
		}
		RefreshPreview(role);
		m_status = "Changed " + std::string(RoleName(role)) + " to " + selected.name;
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
		DrawCatalogBrowser(changed);
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
			m_terrainMaterial->SetUniformFloat("u_materialAmount[" + std::to_string(i) + "]", m_materials[i].amount);
			m_terrainMaterial->SetUniformFloat("u_materialMinHeight[" + std::to_string(i) + "]", m_materials[i].minHeight);
			m_terrainMaterial->SetUniformFloat("u_materialMaxHeight[" + std::to_string(i) + "]", m_materials[i].maxHeight);
			m_terrainMaterial->SetUniformFloat("u_materialMinSlope[" + std::to_string(i) + "]", m_materials[i].minSlope);
			m_terrainMaterial->SetUniformFloat("u_materialMaxSlope[" + std::to_string(i) + "]", m_materials[i].maxSlope);
			m_terrainMaterial->SetUniformFloat("u_materialBlendSoftness[" + std::to_string(i) + "]", m_materials[i].blendSoftness);
		}
	}

	void TerrainMaterialLibrary::MarkLegacySnowPlacement()
	{
		m_needsLegacySnowMigration = true;
	}

	void TerrainMaterialLibrary::MigrateLegacySnowPlacement(float snowHeight, float snowSoftness, float terrainMin, float terrainMax)
	{
		if (!m_needsLegacySnowMigration) return;
		const float range = std::max(terrainMax - terrainMin, 0.001f);
		auto& snow = m_materials[RoleIndex(TerrainMaterialRole::Snow)];
		snow.minHeight = std::clamp((snowHeight - terrainMin) / range, 0.0f, 1.0f);
		snow.maxHeight = 1.0f;
		snow.blendSoftness = std::clamp(snowSoftness / range, 0.001f, 1.0f);
		m_needsLegacySnowMigration = false;
		ApplyMaterialSettings();
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
				if (path.empty()) continue;
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
		if (!std::filesystem::is_directory(folder)) return {};
		for (const auto& entry : std::filesystem::recursive_directory_iterator(folder))
		{
			if (!entry.is_regular_file())
			{
				continue;
			}
			std::string extension = entry.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
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
				if (!ContainsToken(token, "preview") && ContainsToken(filename, "preview")) continue;
				if (ContainsToken(token, "normal") || ContainsToken(token, "nor_"))
				{
					if (ContainsToken(filename, "normaldx") || ContainsToken(filename, "normal_dx") || ContainsToken(filename, "nor_dx")) continue;
				}
				if (ContainsToken(filename, token))
				{
					return NormalizePath(file.string());
				}
			}
		}
		return {};
	}

	PbrMaterialCatalogEntry TerrainMaterialLibrary::DiscoverCatalogEntry(const std::string& folder)
	{
		PbrMaterialCatalogEntry entry;
		if (folder.empty() || !std::filesystem::is_directory(folder))
		{
			entry.validationMessage = "Folder does not exist";
			return entry;
		}
		entry.material.name = std::filesystem::path(folder).filename().string();
		entry.material.folder = NormalizePath(folder);
		entry.material.albedo = FindTextureByToken(folder, { "basecolor", "base_color", "albedo", "color", "diff" });
		entry.material.normal = FindTextureByToken(folder, { "normalgl", "normal-gl", "normal_ogl", "normal-ogl", "nor_gl", "normal" });
		entry.material.roughness = FindTextureByToken(folder, { "roughness", "rough" });
		entry.material.ao = FindTextureByToken(folder, { "ambientocclusion", "ambient_occlusion", "ao" });
		entry.material.height = FindTextureByToken(folder, { "displacement", "height", "disp" });
		entry.preview = FindTextureByToken(folder, { "preview" });
		if (entry.preview.empty()) entry.preview = entry.material.albedo;

		if (entry.material.albedo.empty()) entry.validationMessage = "Missing albedo map";
		else if (entry.material.normal.empty())
		{
			bool unsupportedExr = false;
			for (const auto& file : std::filesystem::recursive_directory_iterator(folder))
			{
				if (!file.is_regular_file()) continue;
				const std::string name = file.path().filename().string();
				std::string extension = file.path().extension().string();
				std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				unsupportedExr = unsupportedExr || (extension == ".exr" && ContainsToken(name, "nor"));
			}
			entry.validationMessage = unsupportedExr ? "Normal map uses unsupported EXR" : "Missing OpenGL normal map";
		}
		else if (entry.material.roughness.empty()) entry.validationMessage = "Missing roughness map";

		if (!entry.validationMessage.empty()) return entry;
		for (const std::string& path : { entry.material.albedo, entry.material.normal, entry.material.roughness, entry.material.ao, entry.material.height })
		{
			if (path.empty()) continue;
			int width = 0;
			int height = 0;
			if (!ReadImageSize(path, width, height))
			{
				entry.validationMessage = "Cannot read " + std::filesystem::path(path).filename().string();
				return entry;
			}
			if (entry.width == 0)
			{
				entry.width = width;
				entry.height = height;
			}
			else if (entry.width != width || entry.height != height)
			{
				entry.validationMessage = "PBR maps use different resolutions";
				return entry;
			}
		}
		entry.valid = true;
		entry.validationMessage = "Ready";
		return entry;
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
		RefreshPreview(TerrainMaterialRole::Snow);
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

		if (ImGui::Button("Change..."))
		{
			m_catalogTargetRole = role;
			m_openCatalog = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Load External Folder..."))
		{
			std::string folder = PickFolderDialog();
			if (!folder.empty())
			{
				changed = LoadPbrFolder(role, folder) || changed;
			}
		}

		if (ImGui::CollapsingHeader("Placement", ImGuiTreeNodeFlags_DefaultOpen))
		{
			changed = ImGui::SliderFloat("Amount", &material.amount, 0.0f, 1.0f, "%.2f") || changed;
			changed = ImGui::DragFloatRange2("Height range", &material.minHeight, &material.maxHeight,
				0.005f, 0.0f, 1.0f, "%.2f", "%.2f") || changed;
			changed = ImGui::DragFloatRange2("Slope range", &material.minSlope, &material.maxSlope,
				0.5f, 0.0f, 90.0f, "%.1f deg", "%.1f deg") || changed;
			changed = ImGui::SliderFloat("Softness", &material.blendSoftness, 0.001f, 0.35f, "%.3f") || changed;
			material.minHeight = std::min(material.minHeight, material.maxHeight);
			material.maxHeight = std::max(material.minHeight, material.maxHeight);
			material.minSlope = std::min(material.minSlope, material.maxSlope);
			material.maxSlope = std::max(material.minSlope, material.maxSlope);
		}

		if (ImGui::CollapsingHeader("Surface", ImGuiTreeNodeFlags_DefaultOpen))
		{
			changed = ImGui::SliderFloat("Tiling", &material.tiling, 0.1f, 8.0f) || changed;
			changed = ImGui::SliderFloat("Normal", &material.normalStrength, 0.0f, 2.0f) || changed;
			changed = ImGui::SliderFloat("Roughness", &material.roughnessScale, 0.1f, 2.0f) || changed;
			changed = ImGui::SliderFloat("Height", &material.heightStrength, 0.0f, 2.0f) || changed;
		}

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

	void TerrainMaterialLibrary::DrawCatalogBrowser(bool& changed)
	{
		if (m_openCatalog)
		{
			ImGui::OpenPopup("PBR Material Browser");
			m_openCatalog = false;
		}
		ImGui::SetNextWindowSize(ImVec2(720.0f, 560.0f), ImGuiCond_FirstUseEver);
		if (!ImGui::BeginPopupModal("PBR Material Browser", nullptr, ImGuiWindowFlags_NoCollapse)) return;

		ImGui::Text("Select material for %s", RoleName(m_catalogTargetRole));
		ImGui::SetNextItemWidth(-110.0f);
		ImGui::InputTextWithHint("##PbrSearch", "Search materials...", m_catalogSearch, sizeof(m_catalogSearch));
		ImGui::SameLine();
		if (ImGui::Button("Refresh")) RefreshCatalog();
		ImGui::Separator();

		std::string search = m_catalogSearch;
		std::transform(search.begin(), search.end(), search.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		const float cardWidth = 132.0f;
		const int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cardWidth));
		if (ImGui::BeginTable("PbrCatalog", columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
			ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing() * 1.8f)))
		{
			for (size_t i = 0; i < m_catalog.size(); ++i)
			{
				const auto& entry = m_catalog[i];
				std::string lowerName = entry.material.name;
				std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				if (!search.empty() && lowerName.find(search) == std::string::npos) continue;
				ImGui::TableNextColumn();
				ImGui::PushID(static_cast<int>(i));
				ImGui::BeginDisabled(!entry.valid);
				bool selected = false;
				if (i < m_catalogPreviews.size() && m_catalogPreviews[i])
				{
					selected = ImGui::ImageButton("##Material", reinterpret_cast<void*>(static_cast<intptr_t>(m_catalogPreviews[i]->GetTextureID())),
						ImVec2(112.0f, 112.0f), ImVec2(0, 1), ImVec2(1, 0));
				}
				else
				{
					selected = ImGui::Button("No preview", ImVec2(112.0f, 112.0f));
				}
				ImGui::TextWrapped("%s", entry.material.name.c_str());
				ImGui::EndDisabled();
				if (!entry.valid && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				{
					ImGui::SetTooltip("%s", entry.validationMessage.c_str());
				}
				if (selected && SelectCatalogMaterial(m_catalogTargetRole, entry))
				{
					changed = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
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

	std::vector<unsigned char> TerrainMaterialLibrary::BuildDataFallbackValues() const
	{
		return {
			255, 255, 255, 255,
			255, 255, 255, 255,
			255, 255, 255, 255,
			128, 128, 128, 128
		};
	}
}
