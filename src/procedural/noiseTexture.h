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
		float scale = 10.0f;
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
		* @param other Reference to another noise texture instance
		*/
		NoiseTexture(const NoiseTexture& other);

		/**
		* @brief Noise texture destructor. Deletes generated textures.
		*/
		~NoiseTexture();

		/**
		* @brief Updates noise parameters with new ones
		* @param noiseParams Reference to a struct of new noise parameters
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

		/**
		* @brief Noise octaves setter
		* @param octaves New number of octaves used for fractal noise generation
		*/
		void SetOctaves(int octaves);

		/**
		* @brief Noise persistance setter
		* @param persistance New persistance value controlling amplitude reduction per octave
		*/
		void SetPersistance(float persistance);

		/**
		* @brief Noise lacunarity setter
		* @param lacunarity New lacunarity value controlling frequency increase per octave
		*/
		void SetLacunarity(float lacunarity);

		/**
		* @brief Noise type setter
		* @param noiseType New FastNoiseLite noise type
		*/
		void SetNoiseType(FastNoiseLite::NoiseType noiseType);

		/**
		* @brief Fractal type setter
		* @param fractalType New FastNoiseLite fractal type
		*/
		void SetFractalType(FastNoiseLite::FractalType fractalType);

		/**
		* @brief Fractal octaves setter
		* @param octaves Number of octaves for fractal noise
		*/
		void SetFractalOctaves(int octaves);

		/**
		* @brief Fractal lacunarity setter
		* @param lacunarity New lacunarity value for fractal noise
		*/
		void SetFractalLacunarity(float lacunarity);

		/**
		* @brief Fractal gain setter
		* @param gain Gain multiplier for fractal noise
		*/
		void SetFractalGain(float gain);

		/**
		* @brief Fractal weighted strength setter
		* @param strength Weighted strength value for fractal noise
		*/
		void SetFractalWeightedStrength(float strength);

		/**
		* @brief Fractal ping-pong strength setter
		* @param strength Ping-pong strength value for fractal noise
		*/
		void SetFractalPingPongStrength(float strength);

		/**
		* @brief Cellular distance function setter
		* @param cellularDistFunction New cellular distance function type
		*/
		void SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction cellularDistFunction);

		/**
		* @brief Cellular return type setter
		* @param cellularReturnType New cellular return type
		*/
		void SetCellularReturnType(FastNoiseLite::CellularReturnType cellularReturnType);

		/**
		* @brief Cellular jitter setter
		* @param jitter Jitter value controlling randomness of cell positions
		*/
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
		unsigned int m_textureID{ 0 };					/**< OpenGL texture ID */
		FastNoiseLite m_noise;							/**< FastNoiseLite noise generator instance */
		NoiseParameters m_noiseParams;					/**< Parameters used for noise generation */
		std::vector<float> m_pixels;					/**< Pixel data of generated noise texture (grayscale) */
		//std::vector<float> m_noiseData;					/**< Raw noise values before normalization */
	};
}
#endif // !SRC_PROCEDURAL_NOISE_TEXTURE_H
