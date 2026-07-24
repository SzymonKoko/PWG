#ifndef SRC_WORLD_OBJECT_RULE_SET_H_
#define SRC_WORLD_OBJECT_RULE_SET_H_

#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace pwg
{
	struct ObjectRuleAsset
	{
		int assetId{ 0 };
		float weight{ 1.0f };
	};

	struct ObjectRuleSet
	{
		int id{ 0 };
		std::string name{ "New Rule Set" };
		int seed{ 1337 };
		bool enabled{ true };
		float density{ 0.35f };
		float spacing{ 80.0f };
		float minHeight{ -20000.0f };
		float maxHeight{ 20000.0f };
		float minSlopeDegrees{ 0.0f };
		float maxSlopeDegrees{ 35.0f };
		std::string maskName;
		float minMaskValue{ 0.0f };
		float maxMaskValue{ 1.0f };
		float avoidWaterDistance{ 0.0f };
		float waterThreshold{ 0.01f };
		float avoidRiverDistance{ 0.0f };
		float riverThreshold{ 0.1f };
		float minScale{ 0.8f };
		float maxScale{ 1.2f };
		glm::vec3 minRotationDegrees{ 0.0f, 0.0f, 0.0f };
		glm::vec3 maxRotationDegrees{ 0.0f, 360.0f, 0.0f };
		bool randomYaw{ true };
		bool alignToNormal{ false };
		std::vector<ObjectRuleAsset> assets;
	};
}

#endif // SRC_WORLD_OBJECT_RULE_SET_H_
