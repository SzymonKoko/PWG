#ifndef SRC_VEGETATION_VEGETATION_SYSTEM_H
#define SRC_VEGETATION_VEGETATION_SYSTEM_H

#include "graphics/material/material.h"
#include "graphics/mesh/mesh.h"
#include "graphics/renderer/renderer.h"
#include "terrain/gpu/terrainComputePipeline.h"
#include "terrain/terrain.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pwg
{
	struct VegetationSubmesh
	{
		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Material> material;
		std::shared_ptr<Texture> albedo;
		glm::vec3 boundsMin{ 0.0f };
		glm::vec3 boundsMax{ 0.0f };
		float groundRadiusRatio{ 0.05f };
		float footprintRadiusRatio{ 0.5f };
	};

	struct VegetationModel
	{
		std::vector<VegetationSubmesh> submeshes;
		glm::vec3 boundsMin{ 0.0f };
		glm::vec3 boundsMax{ 0.0f };
		float targetHeight{ 1.0f };
		float groundRadiusRatio{ 0.05f };
		float footprintRadiusRatio{ 0.5f };
		bool loaded{ false };
	};

	class VegetationSystem
	{
	public:
		VegetationSystem(std::shared_ptr<Shader> vegetationShader, std::shared_ptr<Shader> shadowShader);

		void Sync(std::shared_ptr<Terrain> terrain, const TerrainComputePipeline& pipeline, std::uint64_t terrainRevision);
		void Update(std::shared_ptr<Terrain> terrain, std::shared_ptr<ICamera> camera, float dt);
		void Draw(Renderer& renderer);
		void DrawShadow(const glm::mat4& lightSpaceMatrix);
		void Clear();

		std::size_t GetTreeCount() const;
		std::size_t GetGrassCount() const;
		const std::string& GetStatus() const;

	private:
		struct RuntimeLayer
		{
			VegetationLayerDescriptor descriptor;
			std::shared_ptr<std::vector<float>> densityData;
		};

		struct Instance
		{
			VegetationAssetType assetType{ VegetationAssetType::TreeA };
			glm::vec3 position{ 0.0f };
			glm::mat4 model{ 1.0f };
			float radius{ 1.0f };
			float renderDistance{ 1000.0f };
			float groundRadius{ 0.5f };
			float groundSink{ 0.0f };
			int variantIndex{ 0 };
		};

		bool LoadModel(VegetationAssetType type, const std::string& path, float targetHeight);
		void Regenerate(std::shared_ptr<Terrain> terrain, const std::vector<VegetationLayerDescriptor>& layers);
		void RebuildGrass(std::shared_ptr<Terrain> terrain, const glm::vec3& cameraPosition);
		void RefreshGrounding(std::shared_ptr<Terrain> terrain);
		float SampleGroundHeight(std::shared_ptr<Terrain> terrain, float x, float z, float radius, float sink) const;
		float SampleMask(const std::vector<float>& data, float worldX, float worldZ) const;
		bool AcceptTerrainPoint(std::shared_ptr<Terrain> terrain, float x, float z, float maxSlopeDegrees) const;
		glm::mat4 BuildModelMatrix(const VegetationModel& model, const glm::vec3& position, float yaw, float scale) const;
		glm::mat4 BuildModelMatrix(const VegetationSubmesh& submesh, float targetHeight, const glm::vec3& position, float yaw, float scale) const;
		const VegetationModel* FindModel(VegetationAssetType type) const;

		std::shared_ptr<Shader> m_vegetationShader;
		std::shared_ptr<Shader> m_shadowShader;
		std::unordered_map<int, VegetationModel> m_models;
		std::vector<RuntimeLayer> m_layers;
		std::vector<Instance> m_trees;
		std::vector<Instance> m_grass;
		std::vector<float> m_waterData;
		int m_worldSize{ 0 };
		float m_worldMin{ 0.0f };
		float m_worldMax{ 0.0f };
		std::uint64_t m_seenGraphRevision{ 0 };
		std::uint64_t m_seenTerrainRevision{ 0 };
		glm::vec3 m_lastGrassCamera{ 1000000.0f };
		bool m_grassBuilt{ false };
		float m_time{ 0.0f };
		float m_groundingAccumulator{ 0.0f };
		std::string m_status{ "Vegetation assets are not loaded" };
	};
}

#endif
