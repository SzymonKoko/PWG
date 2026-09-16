#include "waterClipmapRenderer.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "graphics/mesh/mesh.h"
#include "graphics/shader/computeShader.h"
#include "graphics/shader/shader.h"
#include "graphics/texture/texture.h"

namespace pwg
{
	WaterClipmapRenderer::WaterClipmapRenderer(int worldSize, std::shared_ptr<ComputeShader> buildShader, std::shared_ptr<ComputeShader> mipShader)
		: m_worldSize(worldSize), m_buildShader(std::move(buildShader)), m_mipShader(std::move(mipShader))
	{
		m_mipLevels = 1 + static_cast<int>(std::floor(std::log2(std::max(m_worldSize, 1))));
		m_surfaceTexture = CreateMipTexture();
		m_coverageTexture = CreateMipTexture();
		m_rings.reserve(LevelCount);
		for (int level = 0; level < LevelCount; ++level) m_rings.push_back(BuildRingMesh(level == 0));
	}

	WaterClipmapRenderer::~WaterClipmapRenderer()
	{
		if (m_surfaceTexture) glDeleteTextures(1, &m_surfaceTexture);
		if (m_coverageTexture) glDeleteTextures(1, &m_coverageTexture);
	}

	unsigned int WaterClipmapRenderer::CreateMipTexture() const
	{
		unsigned int texture = 0;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexStorage2D(GL_TEXTURE_2D, m_mipLevels, GL_R32F, m_worldSize, m_worldSize);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
		return texture;
	}

	std::shared_ptr<Mesh> WaterClipmapRenderer::BuildRingMesh(bool center) const
	{
		constexpr int half = (GridSize - 1) / 2;
		constexpr int holeHalf = half / 2;
		std::vector<Vertex> vertices(static_cast<size_t>(GridSize) * GridSize);
		for (int y = 0; y < GridSize; ++y)
		{
			for (int x = 0; x < GridSize; ++x)
			{
				auto& vertex = vertices[static_cast<size_t>(y) * GridSize + x];
				vertex.position = glm::vec3(static_cast<float>(x - half), 0.0f, static_cast<float>(y - half));
				vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
				vertex.uv = glm::vec2(static_cast<float>(x), static_cast<float>(y)) / static_cast<float>(GridSize - 1);
				vertex.color = glm::vec3(1.0f);
			}
		}

		std::vector<unsigned int> indices;
		indices.reserve(static_cast<size_t>(GridSize - 1) * (GridSize - 1) * 6);
		for (int y = 0; y < GridSize - 1; ++y)
		{
			for (int x = 0; x < GridSize - 1; ++x)
			{
				int localX = x - half;
				int localY = y - half;
				if (!center && localX >= -holeHalf && localX + 1 <= holeHalf &&
					localY >= -holeHalf && localY + 1 <= holeHalf) continue;
				unsigned int i0 = static_cast<unsigned int>(y * GridSize + x);
				unsigned int i1 = i0 + 1;
				unsigned int i2 = i0 + GridSize;
				unsigned int i3 = i2 + 1;
				indices.insert(indices.end(), { i0, i1, i2, i1, i3, i2 });
			}
		}
		return std::make_shared<Mesh>(vertices, indices, static_cast<float>(GridSize));
	}

	void WaterClipmapRenderer::Invalidate() { m_dirty = true; }

	void WaterClipmapRenderer::Refresh(const std::shared_ptr<Texture>& height, const std::shared_ptr<Texture>& water,
		const std::shared_ptr<Texture>& flow, const std::shared_ptr<Texture>& channels, float minimumDepth)
	{
		if (std::abs(minimumDepth - m_minimumDepth) > 0.00001f)
		{
			m_minimumDepth = minimumDepth;
			m_dirty = true;
		}
		if (!m_dirty || !height || !water || !flow || !channels) return;
		m_buildShader->ActivateShader();
		m_buildShader->SetUniformInt("u_height", 0);
		m_buildShader->SetUniformInt("u_water", 1);
		m_buildShader->SetUniformInt("u_flow", 2);
		m_buildShader->SetUniformInt("u_channels", 3);
		m_buildShader->SetUniformInt("u_size", m_worldSize);
		m_buildShader->SetUniformFloat("u_minimumDepth", minimumDepth);
		m_buildShader->BindTextureSampler(0, height->GetTextureID());
		m_buildShader->BindTextureSampler(1, water->GetTextureID());
		m_buildShader->BindTextureSampler(2, flow->GetTextureID());
		m_buildShader->BindTextureSampler(3, channels->GetTextureID());
		glBindImageTexture(0, m_surfaceTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
		glBindImageTexture(1, m_coverageTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
		m_buildShader->DispatchForTexture(m_worldSize, m_worldSize);

		int sourceWidth = m_worldSize;
		int sourceHeight = m_worldSize;
		for (int level = 1; level < m_mipLevels; ++level)
		{
			int targetWidth = std::max(sourceWidth / 2, 1);
			int targetHeight = std::max(sourceHeight / 2, 1);
			m_mipShader->ActivateShader();
			m_mipShader->SetUniformInt("u_surface", 0);
			m_mipShader->SetUniformInt("u_coverage", 1);
			m_mipShader->SetUniformInt("u_sourceLevel", level - 1);
			m_mipShader->SetUniformVec2("u_sourceSize", glm::vec2(sourceWidth, sourceHeight));
			m_mipShader->SetUniformVec2("u_targetSize", glm::vec2(targetWidth, targetHeight));
			m_mipShader->BindTextureSampler(0, m_surfaceTexture);
			m_mipShader->BindTextureSampler(1, m_coverageTexture);
			glBindImageTexture(0, m_surfaceTexture, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
			glBindImageTexture(1, m_coverageTexture, level, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
			m_mipShader->DispatchForTexture(targetWidth, targetHeight);
			sourceWidth = targetWidth;
			sourceHeight = targetHeight;
		}
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		m_dirty = false;
	}

	void WaterClipmapRenderer::Draw(const std::shared_ptr<Shader>& shader, const glm::vec3& cameraPosition)
	{
		if (!shader || m_dirty) return;
		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, m_surfaceTexture);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, m_coverageTexture);
		shader->SetUniformInt("u_WaterSurfaceCache", 12);
		shader->SetUniformInt("u_WaterCoverageCache", 13);
		shader->SetUniformFloat("u_worldSize", static_cast<float>(m_worldSize));
		shader->SetUniformMat4("u_model", glm::mat4(1.0f));
		constexpr float coarsestSpacing = static_cast<float>(1 << (LevelCount - 1));
		const float halfWorld = static_cast<float>(m_worldSize - 1) * 0.5f;
		const bool overview = std::abs(cameraPosition.x) > halfWorld || std::abs(cameraPosition.z) > halfWorld ||
			cameraPosition.y > static_cast<float>(m_worldSize) * 0.75f;
		const glm::vec2 requestedCenter = overview ? glm::vec2(0.0f) : glm::vec2(cameraPosition.x, cameraPosition.z);
		const glm::vec2 center = glm::floor(requestedCenter / coarsestSpacing) * coarsestSpacing;
		shader->SetUniformVec2("u_clipmapCenter", center);
		for (int level = 0; level < LevelCount; ++level)
		{
			shader->SetUniformFloat("u_gridSpacing", static_cast<float>(1 << level));
			shader->SetUniformInt("u_ringLevel", level);
			m_rings[static_cast<size_t>(level)]->Draw();
		}
	}
}
