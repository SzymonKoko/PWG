#ifndef SRC_CORE_NOISE_NOISE_H
#define SRC_CORE_NOISE_NOISE_H

#include "FastNoiseLite.h"
#include "core/logger/logger.h"

namespace pwg
{
	class Noise
	{
	public:
		Noise();
		~Noise();

		/**
		 * @brief Generating noise height map 
		 * @param x Position on axis X
		 * @param z Position on axis Z
		 */
		float GenerateHeigthMap(float x, float z) const;


		/**
		 * @brief Sets seed of a noise
		 * @param seed Seed that we want to set
		 */
		void SetSeed(int seed);


		/**
		 * @brief Sets frequency of a noise
		 * @param frequency Frequency that we want to set
		 */
		void SetFrequency(float frequency);


		/**
		 * @brief Sets noise type
		 * @param noiseType Type of noise that we want to use
		 */
		void SetNoiseType(FastNoiseLite::NoiseType noiseType);

	private:
		FastNoiseLite m_noise;

	};
}

#endif // !SRC_CORE_NOISE_NOISE_H