#ifndef SRC_TERRAIN_WATER_WATER_CLIPMAP_RENDERER_H
#define SRC_TERRAIN_WATER_WATER_CLIPMAP_RENDERER_H

#include <memory>
#include <vector>
#include <glm/vec3.hpp>

namespace pwg
{
	class ComputeShader;
	class Mesh;
	class Shader;
	class Texture;

	class WaterClipmapRenderer
	{
	public:
		WaterClipmapRenderer(int worldSize, std::shared_ptr<ComputeShader> buildShader, std::shared_ptr<ComputeShader> mipShader);
		~WaterClipmapRenderer();

		void Invalidate();
		void Refresh(const std::shared_ptr<Texture>& height, const std::shared_ptr<Texture>& water,
			const std::shared_ptr<Texture>& flow, const std::shared_ptr<Texture>& channels, float minimumDepth);
		void Draw(const std::shared_ptr<Shader>& shader, const glm::vec3& cameraPosition);

	private:
		std::shared_ptr<Mesh> BuildRingMesh(bool center) const;
		unsigned int CreateMipTexture() const;

		static constexpr int GridSize = 129;
		static constexpr int LevelCount = 6;
		int m_worldSize{ 0 };
		int m_mipLevels{ 1 };
		bool m_dirty{ true };
		float m_minimumDepth{ -1.0f };
		unsigned int m_surfaceTexture{ 0 };
		unsigned int m_coverageTexture{ 0 };
		std::vector<std::shared_ptr<Mesh>> m_rings;
		std::shared_ptr<ComputeShader> m_buildShader;
		std::shared_ptr<ComputeShader> m_mipShader;
	};
}

#endif
