#ifndef SRC_CORE_ECS_ENTITY_H
#define SRC_CORE_ECS_ENTITY_H

#include <entt/entt.hpp>
#include "scene/components/tagComponent.h"
#include "core/logger/logger.h"

namespace pwg
{
	/**
	* @brief Wrapper class around EnTT entity providing utility methods for component management.
	*/
	class Entity
	{
	public:
		/**
		* @brief Default constructor. Creates an empty entity instance without a registry reference.
		*/
		Entity() = default;

		/**
		* @brief Constructs a new entity within the given registry and assigns a name.
		* @param registry Pointer to the EnTT registry that manages this entity.
		* @param name Name assigned to the entity (stored in TagComponent).
		*/
		Entity(entt::registry* registry, std::string name)
			: m_registry(registry),
			m_name(name)
		{
			m_entityHandle = m_registry->create();
			AddComponent<components::TagComponent>(name);
			PWG_INFO("Entity {0} created.", name);
		}

		/**
		* @brief Default destructor. Destroys the entity instance.
		*/
		~Entity() = default;

		/**
		* @brief Adds a new component of type T to the entity.
		* @tparam T Type of the component to add.
		* @tparam Args Constructor argument types.
		* @param args Arguments forwarded to the component's constructor.
		* @return Reference to the newly added component.
		*/
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args);

		/**
		* @brief Retrieves a reference to a component of type T.
		* @tparam T Type of the component to get.
		* @return Reference to the requested component.
		*/
		template<typename T>
		T& GetComponent();

		/**
		* @brief Checks if the entity has a component of type T.
		* @tparam T Type of the component to check.
		* @return True if the entity has the component, false otherwise.
		*/
		template<typename T>
		bool HasComponent();

		/**
		* @brief Removes a component of type T from the entity.
		* @tparam T Type of the component to remove.
		*/
		template<typename T>
		void RemoveComponent();

	private:
		entt::entity m_entityHandle;	/**< EnTT entity handle representing the entity instance */
		entt::registry* m_registry;		/**< Pointer to the registry that manages the entity and its components */
		std::string m_name;				/**< Human-readable name of the entity (also stored in TagComponent) */
	};
}

#include "entity.inl"
#endif // !SRC_CORE_ECS_ENTITY_H
