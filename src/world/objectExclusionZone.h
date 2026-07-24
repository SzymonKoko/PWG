#ifndef SRC_WORLD_OBJECT_EXCLUSION_ZONE_H_
#define SRC_WORLD_OBJECT_EXCLUSION_ZONE_H_

#include <glm/vec2.hpp>
#include <string>

namespace pwg
{
	enum class ObjectExclusionShape
	{
		Circle,
		Box
	};

	struct ObjectExclusionZone
	{
		int id{ 0 };
		std::string name{ "Exclusion Zone" };
		bool enabled{ true };
		int ruleSetId{ 0 };
		ObjectExclusionShape shape{ ObjectExclusionShape::Circle };
		glm::vec2 center{ 0.0f, 0.0f };
		glm::vec2 halfExtents{ 100.0f, 100.0f };
		float radius{ 100.0f };
	};
}


#endif // !SRC_WORLD_OBJECT_EXCLUSION_ZONE_H_
