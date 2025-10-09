#include "noiseDeformer.h"
#include "graphics/mesh/meshManager.h"

void pwg::NoiseDeformer::ApplyNoise(components::PlaneMeshComponent& planeMesh, pwg::Mesh& mesh, const std::vector<float>& noiseData)
{
	std::vector<Vertex> vertices = mesh.GetVertices();

	int width = planeMesh.size;
	int height = planeMesh.size;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int i = y * width + x;
			vertices[i].position.y = noiseData[y * width + x] * 25.f;
		}
	}

	mesh.SetVertices(vertices);
}
