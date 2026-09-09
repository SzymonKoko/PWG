#include "vegetationAssetRegistry.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <yaml-cpp/yaml.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <system_error>
#include <unordered_set>

namespace pwg
{
	namespace
	{
		namespace fs = std::filesystem;

		fs::path LocalApplicationData()
		{
#ifdef _WIN32
			DWORD required = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
			if (required > 1)
			{
				std::wstring value(required, L'\0');
				DWORD written = GetEnvironmentVariableW(L"LOCALAPPDATA", value.data(), required);
				if (written > 0 && written < required)
				{
					value.resize(written);
					return fs::path(value);
				}
			}

			required = GetEnvironmentVariableW(L"USERPROFILE", nullptr, 0);
			if (required > 1)
			{
				std::wstring value(required, L'\0');
				DWORD written = GetEnvironmentVariableW(L"USERPROFILE", value.data(), required);
				if (written > 0 && written < required)
				{
					value.resize(written);
					return fs::path(value) / "AppData" / "Local";
				}
			}
#else
			if (const char* home = std::getenv("HOME"))
			{
				return fs::path(home) / ".local" / "share";
			}
#endif
			std::error_code error;
			fs::path temporary = fs::temp_directory_path(error);
			return error ? fs::current_path() : temporary;
		}

		fs::path BuiltinManifestPath()
		{
			const fs::path runtimePath = fs::path("../assets/vegetation/assets.yaml");
			if (fs::is_regular_file(runtimePath))
			{
				return runtimePath;
			}

			const fs::path sourcePath = fs::path("assets/vegetation/assets.yaml");
			if (fs::is_regular_file(sourcePath))
			{
				return sourcePath;
			}
			return runtimePath;
		}

		std::optional<VegetationKind> ParseKind(const std::string& value)
		{
			std::string lower = value;
			std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char character)
			{
				return static_cast<char>(std::tolower(character));
			});
			if (lower == "tree") return VegetationKind::Tree;
			if (lower == "grass") return VegetationKind::Grass;
			if (lower == "rock") return VegetationKind::Rock;
			return std::nullopt;
		}

		const char* KindName(VegetationKind kind)
		{
			switch (kind)
			{
			case VegetationKind::Grass: return "Grass";
			case VegetationKind::Rock: return "Rock";
			default: return "Tree";
			}
		}

		bool HasSupportedExtension(const fs::path& path)
		{
			std::string extension = path.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character)
			{
				return static_cast<char>(std::tolower(character));
			});
			return extension == ".gltf" || extension == ".glb";
		}

		bool IsGltf(const fs::path& path)
		{
			std::string extension = path.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character)
			{
				return static_cast<char>(std::tolower(character));
			});
			return extension == ".gltf";
		}

		bool IsCompanionFile(const fs::path& path)
		{
			static const std::unordered_set<std::string> extensions = {
				".bin", ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds", ".webp"
			};
			std::string extension = path.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character)
			{
				return static_cast<char>(std::tolower(character));
			});
			return extensions.contains(extension);
		}

		std::string Slug(const std::string& value)
		{
			std::string result;
			bool previousSeparator = false;
			for (unsigned char character : value)
			{
				if (std::isalnum(character))
				{
					result.push_back(static_cast<char>(std::tolower(character)));
					previousSeparator = false;
				}
				else if (!result.empty() && !previousSeparator)
				{
					result.push_back('_');
					previousSeparator = true;
				}
			}
			while (!result.empty() && result.back() == '_') result.pop_back();
			return result.empty() ? "asset" : result;
		}

		std::string UniqueId(const fs::path& libraryRoot, const std::string& name)
		{
			std::random_device randomDevice;
			std::mt19937_64 random(randomDevice());
			const auto now = static_cast<unsigned long long>(
				std::chrono::high_resolution_clock::now().time_since_epoch().count());

			for (int attempt = 0; attempt < 32; ++attempt)
			{
				std::ostringstream suffix;
				suffix << std::hex << (now ^ random());
				std::string id = "user." + Slug(name) + "." + suffix.str().substr(0, 10);
				if (!fs::exists(libraryRoot / id) && !fs::exists(libraryRoot / ("." + id + ".importing")))
				{
					return id;
				}
			}
			return {};
		}

		bool CopyGltfCompanions(const fs::path& sourceModel, const fs::path& destination, std::string& errorMessage)
		{
			std::error_code error;
			const fs::path sourceFolder = sourceModel.parent_path();
			for (const fs::directory_entry& entry : fs::directory_iterator(sourceFolder, error))
			{
				if (error) break;
				if (entry.is_regular_file() && entry.path() != sourceModel && IsCompanionFile(entry.path()))
				{
					fs::copy_file(entry.path(), destination / entry.path().filename(), fs::copy_options::overwrite_existing, error);
					if (error) break;
				}
			}
			if (error)
			{
				errorMessage = "Could not copy a glTF companion file: " + error.message();
				return false;
			}

			const fs::path textures = sourceFolder / "textures";
			if (fs::is_directory(textures, error))
			{
				fs::copy(textures, destination / "textures",
					fs::copy_options::recursive | fs::copy_options::overwrite_existing, error);
				if (error)
				{
					errorMessage = "Could not copy the textures directory: " + error.message();
					return false;
				}
			}
			return true;
		}
	}

	VegetationAssetRegistry::VegetationAssetRegistry()
		: m_libraryRoot(LocalApplicationData() / "PWG" / "AssetLibrary")
	{
		Reload();
	}

	bool VegetationAssetRegistry::Reload()
	{
		m_assets.clear();
		std::vector<std::string> warnings;
		const fs::path builtinManifest = BuiltinManifestPath();
		if (!LoadManifest(builtinManifest, true))
		{
			warnings.push_back("Built-in manifest could not be loaded: " + builtinManifest.string());
		}

		std::error_code error;
		fs::create_directories(m_libraryRoot, error);
		if (error)
		{
			warnings.push_back("Asset library is unavailable: " + error.message());
		}
		else
		{
			for (const fs::directory_entry& entry : fs::directory_iterator(m_libraryRoot, error))
			{
				if (error) break;
				const fs::path manifest = entry.path() / "asset.yaml";
				if (entry.is_directory() && fs::is_regular_file(manifest) && !LoadManifest(manifest, false))
				{
					warnings.push_back("Skipped invalid asset manifest: " + manifest.string());
				}
			}
			if (error) warnings.push_back("Could not enumerate the asset library: " + error.message());
		}

		if (warnings.empty())
		{
			m_status = "Ready: " + std::to_string(m_assets.size()) + " vegetation assets";
			return true;
		}

		std::ostringstream status;
		status << "Loaded " << m_assets.size() << " assets";
		for (const std::string& warning : warnings) status << "; " << warning;
		m_status = status.str();
		return false;
	}

	const std::vector<VegetationAssetDefinition>& VegetationAssetRegistry::Assets() const
	{
		return m_assets;
	}

	const VegetationAssetDefinition* VegetationAssetRegistry::Find(const std::string& id) const
	{
		auto found = std::find_if(m_assets.begin(), m_assets.end(), [&id](const VegetationAssetDefinition& asset)
		{
			return asset.id == id;
		});
		return found == m_assets.end() ? nullptr : &*found;
	}

	std::optional<std::string> VegetationAssetRegistry::Import(
		const fs::path& sourceModel,
		const std::string& name,
		VegetationKind kind,
		float targetHeight,
		float groundSink,
		const glm::vec3& rotationDegrees,
		bool submeshesAsVariants)
	{
		if (!fs::is_regular_file(sourceModel) || !HasSupportedExtension(sourceModel))
		{
			m_status = "Import failed: select an existing .gltf or .glb file";
			return std::nullopt;
		}
		if (name.empty() || targetHeight <= 0.0f || groundSink < 0.0f)
		{
			m_status = "Import failed: name and positive target height are required";
			return std::nullopt;
		}

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(sourceModel.string(),
			aiProcess_Triangulate | aiProcess_ValidateDataStructure);
		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || scene->mNumMeshes == 0)
		{
			m_status = "Import failed: " + std::string(importer.GetErrorString());
			return std::nullopt;
		}

		std::error_code error;
		fs::create_directories(m_libraryRoot, error);
		if (error)
		{
			m_status = "Import failed: could not create asset library: " + error.message();
			return std::nullopt;
		}

		const std::string id = UniqueId(m_libraryRoot, name);
		if (id.empty())
		{
			m_status = "Import failed: could not allocate a unique asset ID";
			return std::nullopt;
		}
		const fs::path staging = m_libraryRoot / ("." + id + ".importing");
		const fs::path destination = m_libraryRoot / id;

		fs::create_directories(staging, error);
		if (!error)
		{
			fs::copy_file(sourceModel, staging / sourceModel.filename(), fs::copy_options::overwrite_existing, error);
		}
		std::string copyError;
		if (!error && IsGltf(sourceModel))
		{
			CopyGltfCompanions(sourceModel, staging, copyError);
		}
		if (error || !copyError.empty())
		{
			std::error_code cleanupError;
			fs::remove_all(staging, cleanupError);
			m_status = "Import failed: " + (copyError.empty() ? error.message() : copyError);
			return std::nullopt;
		}

		try
		{
			YAML::Node manifest;
			manifest["id"] = id;
			manifest["name"] = name;
			manifest["kind"] = KindName(kind);
			manifest["modelPath"] = sourceModel.filename().generic_string();
			manifest["targetHeight"] = targetHeight;
			manifest["groundSink"] = groundSink;
			manifest["rotationX"] = rotationDegrees.x;
			manifest["rotationY"] = rotationDegrees.y;
			manifest["rotationZ"] = rotationDegrees.z;
			manifest["submeshesAsVariants"] = submeshesAsVariants;
			std::ofstream output(staging / "asset.yaml", std::ios::binary);
			if (!output) throw std::runtime_error("could not create asset.yaml");
			output << manifest;
			output.close();
			if (!output) throw std::runtime_error("could not write asset.yaml");

			fs::rename(staging, destination);
		}
		catch (const std::exception& exception)
		{
			std::error_code cleanupError;
			fs::remove_all(staging, cleanupError);
			m_status = "Import failed: " + std::string(exception.what());
			return std::nullopt;
		}

		Reload();
		if (!Find(id))
		{
			m_status = "Import completed, but the new manifest could not be loaded";
			return std::nullopt;
		}
		m_status = "Imported vegetation asset: " + name;
		return id;
	}

	bool VegetationAssetRegistry::Update(
		const std::string& id,
		const std::string& name,
		VegetationKind kind,
		float targetHeight,
		float groundSink,
		const glm::vec3& rotationDegrees,
		bool submeshesAsVariants)
	{
		const VegetationAssetDefinition* current = Find(id);
		if (!current)
		{
			m_status = "Update failed: asset not found";
			return false;
		}
		if (current->builtin)
		{
			m_status = "Update failed: built-in assets are read-only";
			return false;
		}
		if (name.empty() || targetHeight <= 0.0f || groundSink < 0.0f)
		{
			m_status = "Update failed: name and positive target height are required";
			return false;
		}

		const fs::path manifestPath = current->manifestPath;
		std::error_code relativeError;
		fs::path relativeModel = fs::relative(current->modelPath, manifestPath.parent_path(), relativeError);
		if (relativeError) relativeModel = current->modelPath;
		YAML::Node manifest;
		manifest["id"] = id;
		manifest["name"] = name;
		manifest["kind"] = KindName(kind);
		manifest["modelPath"] = relativeModel.generic_string();
		manifest["targetHeight"] = targetHeight;
		manifest["groundSink"] = groundSink;
		manifest["rotationX"] = rotationDegrees.x;
		manifest["rotationY"] = rotationDegrees.y;
		manifest["rotationZ"] = rotationDegrees.z;
		manifest["submeshesAsVariants"] = submeshesAsVariants;

		const fs::path temporaryPath = manifestPath.string() + ".tmp";
		{
			std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
			if (!output)
			{
				m_status = "Update failed: could not create temporary manifest";
				return false;
			}
			output << manifest;
			output.close();
			if (!output)
			{
				std::error_code cleanupError;
				fs::remove(temporaryPath, cleanupError);
				m_status = "Update failed: could not write manifest";
				return false;
			}
		}

#ifdef _WIN32
		if (!MoveFileExW(temporaryPath.c_str(), manifestPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			std::error_code cleanupError;
			fs::remove(temporaryPath, cleanupError);
			m_status = "Update failed: could not replace manifest";
			return false;
		}
#else
		std::error_code replaceError;
		fs::rename(temporaryPath, manifestPath, replaceError);
		if (replaceError)
		{
			fs::remove(temporaryPath, replaceError);
			m_status = "Update failed: could not replace manifest";
			return false;
		}
#endif

		Reload();
		if (!Find(id))
		{
			m_status = "Update completed, but the manifest could not be reloaded";
			return false;
		}
		m_status = "Updated vegetation asset: " + name;
		return true;
	}

	const fs::path& VegetationAssetRegistry::LibraryRoot() const
	{
		return m_libraryRoot;
	}

	const std::string& VegetationAssetRegistry::Status() const
	{
		return m_status;
	}

	bool VegetationAssetRegistry::LoadManifest(const fs::path& manifestPath, bool builtin)
	{
		try
		{
			YAML::Node root = YAML::LoadFile(manifestPath.string());
			YAML::Node entries = root["assets"] ? root["assets"] : root;
			std::vector<YAML::Node> nodes;
			if (entries.IsSequence())
			{
				for (const YAML::Node& entry : entries) nodes.push_back(entry);
			}
			else if (entries.IsMap())
			{
				nodes.push_back(entries);
			}
			else
			{
				return false;
			}

			std::vector<VegetationAssetDefinition> parsed;
			for (const YAML::Node& node : nodes)
			{
				if (!node["id"] || !node["name"] || !node["kind"] || !node["modelPath"])
				{
					return false;
				}
				auto kind = ParseKind(node["kind"].as<std::string>());
				if (!kind) return false;

				VegetationAssetDefinition asset;
				asset.id = node["id"].as<std::string>();
				asset.name = node["name"].as<std::string>();
				asset.kind = *kind;
				fs::path modelPath = fs::path(node["modelPath"].as<std::string>());
				if (!modelPath.is_absolute()) modelPath = manifestPath.parent_path() / modelPath;
				asset.modelPath = fs::absolute(modelPath).lexically_normal().generic_string();
				asset.manifestPath = fs::absolute(manifestPath).lexically_normal().generic_string();
				asset.targetHeight = node["targetHeight"] ? node["targetHeight"].as<float>() : 1.0f;
				asset.groundSink = node["groundSink"] ? node["groundSink"].as<float>() : 0.0f;
				asset.rotationDegrees.x = node["rotationX"] ? node["rotationX"].as<float>() : 0.0f;
				asset.rotationDegrees.y = node["rotationY"] ? node["rotationY"].as<float>() : 0.0f;
				asset.rotationDegrees.z = node["rotationZ"] ? node["rotationZ"].as<float>() : 0.0f;
				asset.submeshesAsVariants = node["submeshesAsVariants"] ? node["submeshesAsVariants"].as<bool>() : false;
				asset.builtin = builtin;

				if (asset.id.empty() || asset.name.empty() || asset.targetHeight <= 0.0f || asset.groundSink < 0.0f ||
					!fs::is_regular_file(asset.modelPath) || Find(asset.id) ||
					std::any_of(parsed.begin(), parsed.end(), [&asset](const VegetationAssetDefinition& item) { return item.id == asset.id; }))
				{
					return false;
				}
				parsed.push_back(std::move(asset));
			}

			m_assets.insert(m_assets.end(), parsed.begin(), parsed.end());
			return true;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}
}
