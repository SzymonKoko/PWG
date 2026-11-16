#include "gpuThermalErosion.h"

#include <algorithm>
#include <cmath>
#include <glad/glad.h>

#include "debug/debugView.h"
#include "graphics/shader/computeShader.h"
#include "graphics/texture/texture.h"

namespace pwg
{
	GpuThermalErosion::GpuThermalErosion(
		int terrainSize,
		std::shared_ptr<ComputeShader> weatheringShader,
		std::shared_ptr<ComputeShader> transportShader,
		std::shared_ptr<ComputeShader> applyShader)
		: m_size(terrainSize),
		  m_weatheringShader(std::move(weatheringShader)),
		  m_transportShader(std::move(transportShader)),
		  m_applyShader(std::move(applyShader))
	{
		m_transferDelta = CreateAtomicTexture(m_size, m_size, GL_R32I);
		m_maxTransfer = CreateAtomicTexture(1, 1, GL_R32UI);
		m_debrisTemp = std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT);
		m_debrisTemp->SetTextureWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	}

	GpuThermalErosion::~GpuThermalErosion()
	{
		if (m_transferDelta) glDeleteTextures(1, &m_transferDelta);
		if (m_maxTransfer) glDeleteTextures(1, &m_maxTransfer);
	}

	unsigned int GpuThermalErosion::CreateAtomicTexture(int width, int height, unsigned int internalFormat)
	{
		unsigned int texture = 0;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);
		glBindTexture(GL_TEXTURE_2D, 0);
		return texture;
	}

	void GpuThermalErosion::ClearAtomicTextures()
	{
		const int zero = 0;
		glClearTexImage(m_transferDelta, 0, GL_RED_INTEGER, GL_INT, &zero);
		glClearTexImage(m_maxTransfer, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	void GpuThermalErosion::Reset(float amount, float talusAngleDegrees, float weathering)
	{
		m_amount = std::clamp(amount, 0.0f, 1.0f);
		m_talusAngleDegrees = std::clamp(talusAngleDegrees, 15.0f, 60.0f);
		m_weathering = std::clamp(weathering, 0.0f, 1.0f);
		m_step = 0;
		m_stableChecks = 0;
		m_stable = false;
		ClearAtomicTextures();
		m_debrisTemp->ClearTexture();
	}

	void GpuThermalErosion::Begin(const std::shared_ptr<Texture>& height)
	{
		if (!height) return;
		auto values = height->ReadFloatData();
		if (values.empty()) return;
		auto [minimum, maximum] = std::minmax_element(values.begin(), values.end());
		m_heightRange = std::max(*maximum - *minimum, 0.001f);
		m_fixedScale = std::clamp(1048576.0f / m_heightRange, 64.0f, 1048576.0f);
	}

	bool GpuThermalErosion::Step(
		const std::shared_ptr<Texture>& height,
		const std::shared_ptr<Texture>& hardness,
		const std::shared_ptr<Texture>& debris,
		const std::shared_ptr<Texture>& debrisMask,
		const std::shared_ptr<Texture>& erosion,
		const std::shared_ptr<Texture>& deposition)
	{
		if (!height || !debris || !debrisMask || !erosion || !deposition || !m_weatheringShader ||
			!m_transportShader || !m_applyShader || m_amount <= 0.0001f || m_stable)
		{
			return false;
		}

		const float talusSlope = std::tan(m_talusAngleDegrees * 0.01745329251994329577f);
		m_weatheringShader->ActivateShader();
		m_weatheringShader->SetUniformInt("u_height", 0);
		m_weatheringShader->SetUniformInt("u_debris", 1);
		m_weatheringShader->SetUniformInt("u_hardness", 2);
		m_weatheringShader->SetUniformInt("u_size", m_size);
		m_weatheringShader->SetUniformFloat("u_amount", m_amount);
		m_weatheringShader->SetUniformFloat("u_weathering", m_weathering);
		m_weatheringShader->SetUniformFloat("u_talusSlope", talusSlope);
		m_weatheringShader->SetUniformInt("u_hasHardness", hardness ? 1 : 0);
		m_weatheringShader->BindTextureSampler(0, height->GetTextureID());
		m_weatheringShader->BindTextureSampler(1, debris->GetTextureID());
		if (hardness) m_weatheringShader->BindTextureSampler(2, hardness->GetTextureID());
		m_weatheringShader->BindImage(0, m_debrisTemp->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_weatheringShader->DispatchForTexture(m_size, m_size);

		ClearAtomicTextures();
		m_transportShader->ActivateShader();
		m_transportShader->SetUniformInt("u_height", 0);
		m_transportShader->SetUniformInt("u_debris", 1);
		m_transportShader->SetUniformInt("u_size", m_size);
		m_transportShader->SetUniformFloat("u_amount", m_amount);
		m_transportShader->SetUniformFloat("u_talusSlope", talusSlope);
		m_transportShader->SetUniformFloat("u_fixedScale", m_fixedScale);
		m_transportShader->BindTextureSampler(0, height->GetTextureID());
		m_transportShader->BindTextureSampler(1, m_debrisTemp->GetTextureID());
		m_transportShader->BindImage(0, m_transferDelta, GL_READ_WRITE, GL_R32I);
		m_transportShader->BindImage(1, m_maxTransfer, GL_READ_WRITE, GL_R32UI);
		m_transportShader->DispatchForTexture(m_size, m_size);

		m_applyShader->ActivateShader();
		m_applyShader->SetUniformInt("u_delta", 0);
		m_applyShader->SetUniformInt("u_debrisSource", 1);
		m_applyShader->SetUniformInt("u_size", m_size);
		m_applyShader->SetUniformFloat("u_fixedScale", m_fixedScale);
		m_applyShader->SetUniformFloat("u_invHeightRange", 1.0f / m_heightRange);
		m_applyShader->BindTextureSampler(0, m_transferDelta);
		m_applyShader->BindTextureSampler(1, m_debrisTemp->GetTextureID());
		m_applyShader->BindImage(0, height->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(1, debris->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_applyShader->BindImage(2, erosion->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(3, deposition->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(4, debrisMask->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_applyShader->DispatchForTexture(m_size, m_size);

		++m_step;
		if ((m_step & 7u) == 0u)
		{
			// Synchronous: glGetTextureImage waits for the step to finish on the GPU (design F7).
			PWG_SIM_EVENT(checkEvent, debug::SimEventType::THERMAL_CHECK);
			unsigned int maximum = 0;
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glGetTextureImage(m_maxTransfer, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, sizeof(maximum), &maximum);
			m_stableChecks = maximum <= 2u ? m_stableChecks + 1 : 0;
			m_stable = m_stableChecks >= 4;
			PWG_SIM_EVENT_SET(checkEvent, solverStep, m_step);
			PWG_SIM_EVENT_SET(checkEvent, stable, m_stable);
			PWG_SIM_EVENT_END(checkEvent);
			// Step returns early once stable, so this is the step at which it became stable.
			if (m_stable) PWG_SIM_CALL(SubmitThermalStable(m_step));
		}
		return true;
	}

	bool GpuThermalErosion::IsStable() const { return m_stable; }
	unsigned int GpuThermalErosion::GetStepCount() const { return m_step; }
}
