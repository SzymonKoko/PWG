#ifndef SRC_SCENE_COMPONENTS_TAG_COMPONENT
#define SRC_SCENE_COMPONENTS_TAG_COMPONENT

#include <string>

namespace pwg::components
{
	/**
	 * @brief Component that stores a human-readable tag or name for an entity.
	 * Useful for identifying entities in logs, editor, or gameplay logic.
	 */
	struct TagComponent
	{
		static constexpr const char* Name = "Tag Component"; /**< Component name identifier. */

		std::string name; /**< Name or tag of the entity. */

		/**
		 * @brief Constructor that sets the entity's tag.
		 * @param name Name or tag for the entity. Defaults to empty string.
		 */
		TagComponent(const std::string& name = "") : name(name) {}
	};
} // namespace pwg::components

#endif // !SRC_SCENE_COMPONENTS_TAG_COMPONENT
