#include "finalMaskPass.h"


namespace pwg
{
	FinalMaskPass::FinalMaskPass(std::shared_ptr<ComputeShader> computeShader, int terrainSize)
		: m_computeShader(computeShader),
		  m_terrainSize(terrainSize)
	{

	}

	void FinalMaskPass::Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks, TerrainPassContext& ctx)
	{
		m_inputHeightTextures.clear();
		m_inputHeightTexturesWeights.clear();

		auto finalMask = masks.at("FinalMask");

		for (const auto& mask : masks)
		{
			if (mask.second->usage == MaskUsage::HEIGHT)
			{
				m_inputHeightTextures.push_back(mask.second->texture);
				m_inputHeightTexturesWeights.push_back(mask.second->weight);
			}
		}

		for (int i = 0; i < m_inputHeightTextures.size(); i++)
		{
			m_inputHeightTextures[i]->Bind(i);
		}

		std::vector<int> textureUnits(m_inputHeightTextures.size());

		for (int i = 0; i < textureUnits.size(); i++)
		{
			textureUnits[i] = i;
		}

		m_computeShader->ActivateShader();

		m_computeShader->SetUniformVectorInt("u_inputTextures", textureUnits);
		m_computeShader->SetUniformVectorFloat("u_inputWeights", m_inputHeightTexturesWeights);
		m_computeShader->SetUniformInt("u_inputsCount", textureUnits.size());

		m_computeShader->BindImage(0, finalMask->texture->GetTextureID(), GL_WRITE_ONLY, GL_R32F);
		m_computeShader->DispatchForTexture(m_terrainSize, m_terrainSize);
		m_computeShader->MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

	}

	std::vector<UIParameters> FinalMaskPass::GetParameters()
	{
		return {};
	}

	std::string FinalMaskPass::GetName()
	{
		return m_name;
	}
}

