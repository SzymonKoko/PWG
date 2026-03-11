#ifndef SRC_TERRAIN_GRAPH_TERRAIN_PASS_H_
#define SRC_TERRAIN_GRAPH_TERRAIN_PASS_H_

#include <string>
#include "graphics/shader/computeShader.h"
#include "graphics/texture/texture.h"
#include "terrain/masks/mask.h"

namespace pwg
{
	enum class TerrainPassTypes
	{
		GENERATION,
		MASK,
		COMPOSITION,
		ANALYSIS,
		SIMULATION
	};
	
	enum class ParameterType
	{
		Float,
		Int,
		Bool,
		Vec2,
		Vec3,
		Mat4,
		List
	};

	struct UIParameters
	{
		std::string name;
		ParameterType type;
		void* data;
		float min;
		float max;
	};

	class TerrainPass
	{
	public:
		virtual void Execute(std::unordered_map<std::string, std::shared_ptr<TerrainMask>>& masks) = 0;
		virtual std::vector<UIParameters> GetParameters() = 0;
		virtual std::string GetName() = 0;
	};
}

#endif // !SRC_TERRAIN_GRAPH_TERRAIN_PASS_H_
