#ifndef SRC_PROCEDURAL_NOISE_TEXTURE_H
#define SRC_PROCEDURAL_NOISE_TEXTURE_H

#include "core/noise/FastNoiseLite.h"
#include <glm/vec2.hpp>
#include <vector>
#include <algorithm>

namespace pwg
{

	struct NoiseParameters
	{
		int seed = 1000;
		float amplitude = 1.0f;
		float frequency = 0.02f;
		float scale = 200.0f;
		int width = 256;
		int height = 256;
		glm::vec2 offset = { 0.0f, 0.0f };
	};

	class NoiseTexture
	{
	public:
		NoiseTexture();
		NoiseTexture(const NoiseParameters& noiseParams);
		NoiseTexture(const NoiseTexture& other);
		~NoiseTexture();

		void UpdateNoiseData(const NoiseParameters& noiseParams);
		NoiseParameters& GetNoiseParameters() { return m_noiseParams; }
		int GetNoiseWidth() { return m_noiseParams.width; }
		int GetNoiseHeight() { return m_noiseParams.height; }
		unsigned int GetTextureID() { return m_textureID; }

		void SetAmplitude(float amplitude);
		void SetFrequency(float frequency);
		void SetScale(float scale);
		void SetOffset(glm::vec2 offset);
		void SetSeed(int seed);

	private:
		void GenerateNoiseData();
		void UploadToGPU();
	private:
		unsigned int m_textureID{ 0 };
		FastNoiseLite m_noise;
		NoiseParameters m_noiseParams;
		std::vector<unsigned char> m_pixels;

	};
}
#endif // !SRC_PROCEDURAL_NOISE_TEXTURE_H
