#include "noise.h"

pwg::Noise::Noise()
{
	/*m_noise.SetSeed(1200);
	m_noise.SetFrequency(1.5f);
	m_noise.SetNoiseType(FastNoiseLite::NoiseType_ValueCubic);*/
	m_noise.SetSeed(42);
	m_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_noise.SetFrequency(0.005f);
	m_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
	m_noise.SetFractalOctaves(5);
	m_noise.SetFractalLacunarity(2.0f);
	m_noise.SetFractalGain(0.3f);
}

pwg::Noise::~Noise()
{
}

float pwg::Noise::GenerateHeigthMap(float x, float z) const
{
	return m_noise.GetNoise(x, z);
}

void pwg::Noise::SetSeed(int seed)
{
	m_noise.SetSeed(seed);
}

void pwg::Noise::SetFrequency(float frequency)
{
	m_noise.SetFrequency(frequency);
}

void pwg::Noise::SetNoiseType(FastNoiseLite::NoiseType noiseType) 
{
	m_noise.SetNoiseType(noiseType);
}
