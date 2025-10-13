#include "noiseDeformer.h"
#include "graphics/mesh/meshManager.h"

void pwg::NoiseDeformer::ApplyNoise(pwg::Mesh& mesh, const std::vector<float>& noiseData)
{
	std::vector<Vertex> vertices = mesh.GetVertices();

	int size = mesh.GetSize();

	for (int y = 0; y < size; y++)
	{
		for (int x = 0; x < size; x++)
		{
			int i = y * size + x;
			vertices[i].position.y = noiseData[y * size + x] * 25.f;
		}
	}

	mesh.SetVertices(vertices);
}
