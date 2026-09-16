#ifndef SRC_VEGETATION_VEGETATION_SYSTEM_H
#define SRC_VEGETATION_VEGETATION_SYSTEM_H

#include "graphics/material/material.h"
#include "graphics/mesh/mesh.h"
#include "graphics/renderer/renderer.h"
#include "terrain/gpu/terrainComputePipeline.h"
#include "terrain/terrain.h"
#include "vegetation/vegetationAssetRegistry.h"

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
		std::string assetId;
		VegetationKind kind{ VegetationKind::Tree };
		std::vector<VegetationSubmesh> submeshes;
		glm::vec3 boundsMin{ 0.0f };
		glm::vec3 boundsMax{ 0.0f };
		float targetHeight{ 1.0f };
		float groundSink{ 0.1f };
		float groundRadiusRatio{ 0.05f };
		float footprintRadiusRatio{ 0.5f };
		bool submeshesAsVariants{ false };
		bool loaded{ false };
	};

	class VegetationSystem
	{
	public:
		VegetationSystem(std::shared_ptr<Shader> vegetationShader, std::shared_ptr<Shader> shadowShader, std::shared_ptr<VegetationAssetRegistry> assetRegistry);

		void Sync(std::shared_ptr<Terrain> terrain, const TerrainComputePipeline& pipeline, std::uint64_t terrainRevision);
		void Update(std::shared_ptr<Terrain> terrain, std::shared_ptr<ICamera> camera, float dt);
		void Draw(Renderer& renderer, std::shared_ptr<Terrain> terrain);
		void DrawShadow(const glm::mat4& lightSpaceMatrix);
		float GetCollisionGroundHeight(std::shared_ptr<Terrain> terrain, float x, float z) const;
		void ReloadAssetModels();
		void Clear();

		std::size_t GetTreeCount() const;
		std::size_t GetGrassCount() const;
		const std::string& GetStatus() const;

	private:
		struct RuntimeLayer
		{
			VegetationLayerDescriptor descriptor;
			std::shared_ptr<std::vector<float>> densityData;
			int modelIndex{ -1 };
		};

		struct Instance
		{
			int modelIndex{ -1 };
			glm::vec3 position{ 0.0f };
			glm::mat4 model{ 1.0f };
			float radius{ 1.0f };
			float renderDistance{ 1000.0f };
			float groundRadius{ 0.5f };
			float groundSink{ 0.0f };
			float collisionRadius{ 0.0f };
			float collisionHeight{ 0.0f };
			int variantIndex{ 0 };
			float yaw{ 0.0f };
			float scale{ 1.0f };
			float maxTiltDegrees{ 0.0f };
			bool alignToSlope{ false };
		};

		bool LoadModel(const VegetationAssetDefinition& definition);
		int EnsureModel(const std::string& assetId);
		void Regenerate(std::shared_ptr<Terrain> terrain, const std::vector<VegetationLayerDescriptor>& layers);
		void RebuildGrass(std::shared_ptr<Terrain> terrain, const glm::vec3& cameraPosition);
		void RefreshGrounding(std::shared_ptr<Terrain> terrain);
		float SampleGroundHeight(std::shared_ptr<Terrain> terrain, float x, float z, float radius, float sink) const;
		float SampleMask(const std::vector<float>& data, float worldX, float worldZ) const;
		bool AcceptTerrainPoint(std::shared_ptr<Terrain> terrain, float x, float z, float maxSlopeDegrees, bool avoidWater) const;
		glm::vec3 SampleTerrainNormal(std::shared_ptr<Terrain> terrain, float x, float z) const;
		glm::mat4 BuildModelMatrix(const VegetationModel& model, const glm::vec3& position, float yaw, float scale, const glm::vec3& normal, float maxTiltDegrees) const;
		glm::mat4 BuildModelMatrix(const VegetationSubmesh& submesh, float targetHeight, const glm::vec3& position, float yaw, float scale, const glm::vec3& normal, float maxTiltDegrees) const;
		const VegetationModel* FindModel(int modelIndex) const;

		std::shared_ptr<Shader> m_vegetationShader;
		std::shared_ptr<Shader> m_shadowShader;
		std::shared_ptr<VegetationAssetRegistry> m_assetRegistry;
		std::vector<VegetationModel> m_models;
		std::unordered_map<std::string, int> m_modelIndices;
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
		float m_minVisibleWaterDepth{ 0.025f };
		std::string m_status{ "Vegetation assets are not loaded" };
	};
}

#endif
