#ifndef SRC_TERRAIN_EROSION_GPU_PARTICLE_EROSION_H
#define SRC_TERRAIN_EROSION_GPU_PARTICLE_EROSION_H

#include <memory>

namespace pwg
{
	class ComputeShader;
	class Shader;
	class Texture;

	class GpuParticleErosion
	{
	public:
		GpuParticleErosion(
			int terrainSize,
			std::shared_ptr<ComputeShader> particleShader,
			std::shared_ptr<ComputeShader> applyShader,
			std::shared_ptr<ComputeShader> talusShader,
			std::shared_ptr<Shader> debugShader);
		~GpuParticleErosion();

		void Reset(int seed, float amount);
		void Step(
			const std::shared_ptr<Texture>& height,
			const std::shared_ptr<Texture>& water,
			const std::shared_ptr<Texture>& flow,
			const std::shared_ptr<Texture>& channels,
			const std::shared_ptr<Texture>& deposition,
			const std::shared_ptr<Texture>& erosion,
			const std::shared_ptr<Texture>& erosionDelta);
		void DrawDebug(const float* view, const float* projection, const std::shared_ptr<Texture>& height);

		void SetAmount(float amount);
		float GetAmount() const;
		int GetParticleCount() const;

	private:
		unsigned int CreateAtomicTexture();
		void ClearAtomicTextures();

		int m_size{ 0 };
		int m_particleCapacity{ 0 };
		int m_activeParticles{ 0 };
		int m_seed{ 1337 };
		unsigned int m_step{ 0 };
		float m_amount{ 0.35f };
		unsigned int m_particleBuffer{ 0 };
		unsigned int m_debugVao{ 0 };
		unsigned int m_heightDelta{ 0 };
		unsigned int m_flowDelta{ 0 };
		unsigned int m_waterDelta{ 0 };
		std::shared_ptr<ComputeShader> m_particleShader;
		std::shared_ptr<ComputeShader> m_applyShader;
		std::shared_ptr<ComputeShader> m_talusShader;
		std::shared_ptr<Shader> m_debugShader;
		std::shared_ptr<Texture> m_talusTemp;
	};
}

#endif
