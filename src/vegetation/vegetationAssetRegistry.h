#ifndef SRC_VEGETATION_VEGETATION_ASSET_REGISTRY_H
#define SRC_VEGETATION_VEGETATION_ASSET_REGISTRY_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace pwg
{
	enum class VegetationKind
	{
		Tree,
		Grass,
		Rock
	};

	struct VegetationAssetDefinition
	{
		std::string id;
		std::string name;
		VegetationKind kind{ VegetationKind::Tree };
		std::string modelPath;
		std::string manifestPath;
		float targetHeight{ 1.0f };
		float groundSink{ 0.0f };
		bool submeshesAsVariants{ false };
		bool builtin{ false };
	};

	class VegetationAssetRegistry
	{
	public:
		VegetationAssetRegistry();

		bool Reload();
		const std::vector<VegetationAssetDefinition>& Assets() const;
		const VegetationAssetDefinition* Find(const std::string& id) const;

		std::optional<std::string> Import(
			const std::filesystem::path& sourceModel,
			const std::string& name,
			VegetationKind kind,
			float targetHeight,
			float groundSink,
			bool submeshesAsVariants);
		bool Update(
			const std::string& id,
			const std::string& name,
			VegetationKind kind,
			float targetHeight,
			float groundSink,
			bool submeshesAsVariants);

		const std::filesystem::path& LibraryRoot() const;
		const std::string& Status() const;

	private:
		bool LoadManifest(const std::filesystem::path& manifestPath, bool builtin);

		std::filesystem::path m_libraryRoot;
		std::vector<VegetationAssetDefinition> m_assets;
		std::string m_status;
	};
}

#endif
