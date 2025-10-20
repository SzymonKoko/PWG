#ifndef SRC_PROCEDURAL_NOISE_TEXTURE_H
#define SRC_PROCEDURAL_NOISE_TEXTURE_H

#include "FastNoiseLite/FastNoiseLite.h"
#include <glm/vec2.hpp>
#include <vector>
#include <algorithm>

namespace pwg
{
	/**
	* @brief Struct that holds parameters of the noise
	*/
	struct NoiseParameters
	{
		int seed = 1000;
		float amplitude = 1.0f;
		float frequency = 0.02f;
		float scale = 100.0f;
		int octaves = 4;
		float persistance = 0.5f;
		float lacunarity = 2.0f;
		int size = 100;
		glm::vec2 offset = { 0.0f, 0.0f };
	};

	class NoiseTexture
	{
	public:

		/**
		* @brief Noise texture constructor. Constructs noise with default parameters
		*/
		NoiseTexture(int terrainSize);

		/**
		* @brief Noise texture constructor. Constructs noise with new parameters
		* @param noiseParams Reference to a new parameters of the noise
		*/
		NoiseTexture(const NoiseParameters& noiseParams);

		/**
		* @brief Noise texture copy constructor. Constructs a copy of the noise texture instance
		* @params Reference to a other noise texture instance
		*/
		NoiseTexture(const NoiseTexture& other);

		/**
		* @brief Noise texture destructor. Deletes generated textures.
		*/
		~NoiseTexture();

		/**
		* @brief Updates noise parameters with new ones
		* @params noiseParams Reference to a struct of new noise parameters
		*/
		void UpdateNoiseData(const NoiseParameters& noiseParams);

		/**
		* @brief Noise parameters getter
		* @return Returns reference to struct of noise parameters
		*/
		NoiseParameters& GetNoiseParameters() { return m_noiseParams; }

		/**
		* @brief Noise width getter
		* @return Returns width of a noise texture in pixels
		*/
		int GetNoiseWidth() { return m_noiseParams.size; }

		/**
		* @brief Noise height getter
		* @return Returns height of a noise texture in pixels
		*/
		int GetNoiseHeight() { return m_noiseParams.size; }

		/**
		* @brief Noise texture ID getter
		* @return Returns id of the noise texture
		*/
		unsigned int GetTextureID() { return m_textureID; }

		/**
		* @brief Noise data getter
		* @return Returns vector of noise data float values
		*/
		std::vector<float> GetNoiseData() { return m_noiseData; }

		/**
		* @brief Noise amplitude setter
		* @param amplitude New amplitude of a noise
		*/
		void SetAmplitude(float amplitude);

		/**
		* @brief Noise frequency setter
		* @param frequency New frequency of a noise
		*/
		void SetFrequency(float frequency);

		/**
		* @brief Noise scale setter
		* @param scale New scale of a noise
		*/
		void SetScale(float scale);

		/**
		* @brief Noise offset setter
		* @param offset New offset of a noise
		*/
		void SetOffset(glm::vec2 offset);

		/**
		* @brief Noise seed setter
		* @param seed New seed of a noise
		*/
		void SetSeed(int seed);

		void SetOctaves(int octaves);

		void SetPersistance(float persistance);

		void SetLacunarity(float lacunarity);

		void SetNoiseType(FastNoiseLite::NoiseType noiseType);

		void SetFractalType(FastNoiseLite::FractalType fractalType);

		void SetFractalOctaves(int octaves);

		void SetFractalLacunarity(float lacunarity);

		void SetFractalGain(float gain);

		void SetFractalWeightedStrength(float strength);

		void SetFractalPingPongStrength(float strength);

		void SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction cellularDistFunction);

		void SetCellularReturnType(FastNoiseLite::CellularReturnType cellularReturnType);

		void SetCellularJitter(float jitter);

	private:

		/**
		* @brief Generates noise based on a noise parameters. Creates texture of the noise
		*/
		void GenerateNoiseData();

		/**
		* @brief Uploads noise texture to GPU
		*/
		void UploadToGPU();
	private:
		unsigned int m_textureID{ 0 };
		FastNoiseLite m_noise;
		NoiseParameters m_noiseParams;
		std::vector<unsigned char> m_pixels;
		std::vector<float> m_noiseData;

		

	};
}
#endif // !SRC_PROCEDURAL_NOISE_TEXTURE_H
