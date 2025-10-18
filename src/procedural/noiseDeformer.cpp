#include "noiseDeformer.h"
#include "graphics/mesh/meshManager.h"

void pwg::NoiseDeformer::ApplyNoise(pwg::Mesh& mesh, const std::vector<float>& noiseData)
{
	std::vector<Vertex> vertices = mesh.GetVertices();

	int size = mesh.GetSize();

	float m_minNoiseHeight{ 0 };
	float m_maxNoiseHeight{ 0 };

	for (int y = 0; y < size; y++)
	{
		for (int x = 0; x < size; x++)
		{
			int i = y * size + x;
			vertices[i].position.y = noiseData[y * size + x] * 25.f;

			if (vertices[i].position.y <= m_minNoiseHeight)
			{
				m_minNoiseHeight = vertices[i].position.y;
			}
			if (vertices[i].position.y >= m_maxNoiseHeight)
			{
				m_maxNoiseHeight = vertices[i].position.y;
			}
		}
	}

	std::cout << "Min: " << m_minNoiseHeight << "	Max: " << m_maxNoiseHeight << std::endl;

	mesh.SetVertices(vertices);
}
