#ifndef SRC_TERRAIN_EROSION_GPU_THERMAL_EROSION_H
#define SRC_TERRAIN_EROSION_GPU_THERMAL_EROSION_H

#include <memory>

namespace pwg
{
	class ComputeShader;
	class Texture;

	class GpuThermalErosion
	{
	public:
		GpuThermalErosion(
			int terrainSize,
			std::shared_ptr<ComputeShader> weatheringShader,
			std::shared_ptr<ComputeShader> transportShader,
			std::shared_ptr<ComputeShader> applyShader);
		~GpuThermalErosion();

		void Reset(float amount, float talusAngleDegrees, float weathering);
		void Begin(const std::shared_ptr<Texture>& height);
		bool Step(
			const std::shared_ptr<Texture>& height,
			const std::shared_ptr<Texture>& hardness,
			const std::shared_ptr<Texture>& debris,
			const std::shared_ptr<Texture>& debrisMask,
			const std::shared_ptr<Texture>& erosion,
			const std::shared_ptr<Texture>& deposition);

		bool IsStable() const;
		unsigned int GetStepCount() const;

	private:
		unsigned int CreateAtomicTexture(int width, int height, unsigned int internalFormat);
		void ClearAtomicTextures();

		int m_size{ 0 };
		float m_amount{ 0.35f };
		float m_talusAngleDegrees{ 32.0f };
		float m_weathering{ 0.45f };
		float m_fixedScale{ 65536.0f };
		float m_heightRange{ 1.0f };
		unsigned int m_step{ 0 };
		int m_stableChecks{ 0 };
		bool m_stable{ false };
		unsigned int m_transferDelta{ 0 };
		unsigned int m_maxTransfer{ 0 };
		std::shared_ptr<Texture> m_debrisTemp;
		std::shared_ptr<ComputeShader> m_weatheringShader;
		std::shared_ptr<ComputeShader> m_transportShader;
		std::shared_ptr<ComputeShader> m_applyShader;
	};
}

#endif
