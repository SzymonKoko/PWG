#include "gpuParticleErosion.h"

#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "debug/debugView.h"
#include "graphics/shader/computeShader.h"
#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"

namespace pwg
{
	GpuParticleErosion::GpuParticleErosion(
		int terrainSize,
		std::shared_ptr<ComputeShader> particleShader,
		std::shared_ptr<ComputeShader> applyShader,
		std::shared_ptr<ComputeShader> talusShader,
		std::shared_ptr<Shader> debugShader)
		: m_size(terrainSize),
		  m_particleShader(std::move(particleShader)),
		  m_applyShader(std::move(applyShader)),
		  m_talusShader(std::move(talusShader)),
		  m_debugShader(std::move(debugShader))
	{
		const long long pixels = static_cast<long long>(m_size) * m_size;
		m_particleCapacity = static_cast<int>(std::clamp(pixels / 64LL, 32768LL, 262144LL));
		glGenBuffers(1, &m_particleBuffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particleBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(m_particleCapacity) * 32, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glGenVertexArrays(1, &m_debugVao);
		m_heightDelta = CreateAtomicTexture();
		m_flowDelta = CreateAtomicTexture();
		m_waterDelta = CreateAtomicTexture();
		m_talusTemp = std::make_shared<Texture>(m_size, m_size, TextureType::HEIGHT);
		m_talusTemp->SetTextureWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		Reset(m_seed, m_amount);
	}

	GpuParticleErosion::~GpuParticleErosion()
	{
		if (m_heightDelta) glDeleteTextures(1, &m_heightDelta);
		if (m_flowDelta) glDeleteTextures(1, &m_flowDelta);
		if (m_waterDelta) glDeleteTextures(1, &m_waterDelta);
		if (m_particleBuffer) glDeleteBuffers(1, &m_particleBuffer);
		if (m_debugVao) glDeleteVertexArrays(1, &m_debugVao);
	}

	unsigned int GpuParticleErosion::CreateAtomicTexture()
	{
		unsigned int texture = 0;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32I, m_size, m_size);
		glBindTexture(GL_TEXTURE_2D, 0);
		return texture;
	}

	void GpuParticleErosion::ClearAtomicTextures()
	{
		const int zero = 0;
		for (unsigned int texture : { m_heightDelta, m_flowDelta, m_waterDelta })
		{
			glClearTexImage(texture, 0, GL_RED_INTEGER, GL_INT, &zero);
		}
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	void GpuParticleErosion::Reset(int seed, float amount)
	{
		m_seed = seed;
		m_step = 0;
		SetAmount(amount);
		const unsigned int zero = 0;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particleBuffer);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		ClearAtomicTextures();
	}

	void GpuParticleErosion::SetAmount(float amount)
	{
		m_amount = std::clamp(amount, 0.0f, 1.0f);
		float activeRatio = m_amount <= 0.0001f ? 0.0f : std::lerp(0.22f, 1.0f, m_amount * m_amount * (3.0f - 2.0f * m_amount));
		m_activeParticles = static_cast<int>(std::round(m_particleCapacity * activeRatio));
	}

	float GpuParticleErosion::GetAmount() const { return m_amount; }
	int GpuParticleErosion::GetParticleCount() const { return m_activeParticles; }

	void GpuParticleErosion::Step(
		const std::shared_ptr<Texture>& height,
		const std::shared_ptr<Texture>& water,
		const std::shared_ptr<Texture>& flow,
		const std::shared_ptr<Texture>& channels,
		const std::shared_ptr<Texture>& deposition,
		const std::shared_ptr<Texture>& erosion,
		const std::shared_ptr<Texture>& erosionDelta)
	{
		if (!m_particleShader || !m_applyShader || m_activeParticles <= 0) return;

		m_particleShader->ActivateShader();
		m_particleShader->SetUniformInt("u_height", 0);
		m_particleShader->SetUniformInt("u_water", 1);
		m_particleShader->SetUniformInt("u_flow", 2);
		m_particleShader->SetUniformInt("u_channels", 3);
		m_particleShader->SetUniformInt("u_size", m_size);
		m_particleShader->SetUniformInt("u_particleCount", m_activeParticles);
		m_particleShader->SetUniformInt("u_seed", m_seed);
		m_particleShader->SetUniformInt("u_step", static_cast<int>(m_step));
		m_particleShader->SetUniformFloat("u_amount", m_amount);
		m_particleShader->BindTextureSampler(0, height->GetTextureID());
		m_particleShader->BindTextureSampler(1, water->GetTextureID());
		m_particleShader->BindTextureSampler(2, flow->GetTextureID());
		m_particleShader->BindTextureSampler(3, channels->GetTextureID());
		m_particleShader->BindImage(0, m_heightDelta, GL_READ_WRITE, GL_R32I);
		m_particleShader->BindImage(1, m_flowDelta, GL_READ_WRITE, GL_R32I);
		m_particleShader->BindImage(2, m_waterDelta, GL_READ_WRITE, GL_R32I);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_particleBuffer);
		m_particleShader->Dispatch((m_activeParticles + 255) / 256);
		m_particleShader->MemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		m_applyShader->ActivateShader();
		m_applyShader->SetUniformInt("u_size", m_size);
		m_applyShader->SetUniformFloat("u_amount", m_amount);
		m_applyShader->SetUniformInt("u_heightDelta", 0);
		m_applyShader->SetUniformInt("u_flowDelta", 1);
		m_applyShader->SetUniformInt("u_waterDelta", 2);
		m_applyShader->BindTextureSampler(0, m_heightDelta);
		m_applyShader->BindTextureSampler(1, m_flowDelta);
		m_applyShader->BindTextureSampler(2, m_waterDelta);
		m_applyShader->BindImage(0, height->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(1, water->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(2, flow->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(3, channels->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(4, deposition->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(5, erosion->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->BindImage(6, erosionDelta->GetTextureID(), GL_READ_WRITE, GL_R32F);
		m_applyShader->DispatchForTexture(m_size, m_size);
		ClearAtomicTextures();
		++m_step;

		if (m_talusShader && (m_step & 7u) == 0u)
		{
			for (int iteration = 0; iteration < 2; ++iteration)
			{
				m_talusShader->ActivateShader();
				m_talusShader->SetUniformInt("u_height", 0);
				m_talusShader->SetUniformInt("u_size", m_size);
				m_talusShader->SetUniformFloat("u_amount", m_amount);
				m_talusShader->BindTextureSampler(0, height->GetTextureID());
				m_talusShader->BindImage(0, m_talusTemp->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
				m_talusShader->DispatchForTexture(m_size, m_size);
				height->CopyFrom(*m_talusTemp);
			}
		}
	}

	void GpuParticleErosion::DrawDebug(const float* view, const float* projection, const std::shared_ptr<Texture>& height)
	{
		if (!m_debugShader || !height || m_activeParticles <= 0) return;
		m_debugShader->ActivateShader();
		m_debugShader->SetUniformInt("u_height", 0);
		m_debugShader->SetUniformInt("u_size", m_size);
		glUniformMatrix4fv(m_debugShader->GetUniformLocation("u_view"), 1, GL_FALSE, view);
		glUniformMatrix4fv(m_debugShader->GetUniformLocation("u_projection"), 1, GL_FALSE, projection);
		height->Bind(0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_particleBuffer);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
		glBindVertexArray(m_debugVao);
		glEnable(GL_PROGRAM_POINT_SIZE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_POINTS, 0, m_activeParticles);
		// One draw command; points are not triangles, so it adds none.
		PWG_RENDER_COUNT(AddDraw(0));
		glDisable(GL_BLEND);
		glBindVertexArray(0);
	}
}
