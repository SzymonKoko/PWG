#ifndef SRC_TERRAIN_LOD_LOD_MANAGER_H_
#define SRC_TERRAIN_LOD_LOD_MANAGER_H_

#include <array>
#include <memory>

#include "terrain/lod/LODDistances.h"
#include "graphics/mesh/meshManager.h"
#include "terrain/chunk/terrainChunk.h"

#define MAX_LOD 5

namespace pwg
{
	class LODManager
	{
	public:
		LODManager(int chunkSize);
		~LODManager() = default;

		/** @brief Number of switch thresholds: LOD0..LOD3 end at one each, LOD4 takes everything beyond the last. */
		static constexpr int ThresholdCount = MAX_LOD - 1;
		using Thresholds = std::array<float, ThresholdCount>;

		/**
		* @brief Runtime LOD controls (Debug View, D-05). The defaults reproduce the fixed behaviour:
		* enabled, scale 1, the distances of LODDistances.
		*/
		struct Settings
		{
			bool enabled{ true };				/**< false: every chunk uses LOD0 ("force LOD0"). */
			float scale{ 1.0f };				/**< Multiplies every threshold. */
			Thresholds distances{ LODDistances{}.lod0.second, LODDistances{}.lod1.second, LODDistances{}.lod2.second, LODDistances{}.lod3.second };
		};

		void GenerateLODMeshes();
		void SelectLOD(TerrainChunk& chunk, float distance);

		/** @brief Default thresholds (the values of LODDistances). */
		static Thresholds GetDefaultDistances() { return Settings{}.distances; }

		void SetEnabled(bool enabled) { m_settings.enabled = enabled; }
		bool IsEnabled() const { return m_settings.enabled; }

		/** @brief Threshold scale, finite and in (0, MaxScale]. Returns false and keeps the old value otherwise. */
		bool SetScale(float scale);
		float GetScale() const { return m_settings.scale; }
		static constexpr float MaxScale = 100.0f;

		/** @brief Switch distances of LOD0..LOD3 in world units: finite, >= 0, not decreasing. Returns false and keeps the old values otherwise. */
		bool SetDistances(const Thresholds& distances);
		const Thresholds& GetDistances() const { return m_settings.distances; }
		void ResetDistances() { m_settings.distances = GetDefaultDistances(); }

		const Settings& GetSettings() const { return m_settings; }
		void SetSettings(const Settings& settings) { m_settings = settings; }

		std::shared_ptr<Mesh> GetMesh(int lod);
	private:
		std::shared_ptr<Mesh> m_lodMeshes[MAX_LOD];
		Settings m_settings;
		LODDistances m_lodDistances;		/**< Level numbers (and the defaults the settings start from). */
		int m_chunkSize{ 0 };
	};
}
#endif // !SRC_TERRAIN_LOD_LOD_MANAGER_H_
