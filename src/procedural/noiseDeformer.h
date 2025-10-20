#ifndef SRC_PROCEDURAL_NOISE_DEFORMER_H
#define SRC_PROCEDURAL_NOISE_DEFORMER_H

#include "graphics/mesh/mesh.h"
#include "scene/components/planeMeshComponent.h"
#include "scene/components/meshComponent.h"
#include "core/ecs/entity.h"

#include <vector>

namespace pwg
{
	/**
	 * @brief Class responsible for deforming meshes using procedural noise.
	 * Provides static methods to apply noise data to mesh vertices.
	 */
	class NoiseDeformer
	{
	public:

		/**
		 * @brief Default constructor.
		 */
		NoiseDeformer() = default;

		/**
		 * @brief Destructor.
		 */
		~NoiseDeformer() = default;

		/**
		 * @brief Applies procedural noise to a mesh's vertices.
		 * Modifies the mesh in-place based on the provided noise data.
		 * @param mesh Reference to the mesh to deform.
		 * @param noiseData Vector of float values representing procedural noise.
		 */
		static void ApplyNoise(pwg::Mesh& mesh, const std::vector<float>& noiseData);

	private:
		// No private members
	};
} // namespace pwg

#endif // !SRC_PROCEDURAL_NOISE_DEFORMER_H
