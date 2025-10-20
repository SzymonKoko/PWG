#ifndef SRC_PROCEDURAL_NOISE_DEFORMER_H
#define SRC_PROCEDURAL_NOISE_DEFORMER_H

#include "graphics/mesh/mesh.h"
#include "scene/components/planeMeshComponent.h"
#include "scene/components/meshComponent.h"
#include "core/ecs/entity.h"

#include <vector>

namespace pwg
{
	class NoiseDeformer
	{
	public:
		NoiseDeformer() = default;
		~NoiseDeformer() = default;

		static void ApplyNoise(pwg::Mesh& mesh, const std::vector<float>& noiseData);
	private:
	};
}
#endif // !SRC_PROCEDURAL_NOISE_DEFORMER_H
