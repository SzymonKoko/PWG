#ifndef SRC_CORE_ECS_ENTITY_H
#define SRC_CORE_ECS_ENTITY_H

#include <entt/entt.hpp>
#include "scene/components/tagComponent.h"
#include "core/logger/logger.h"

namespace pwg
{
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::registry* registry, std::string name) 
			: m_registry(registry),
			  m_name(name)
		{
			m_entityHandle = m_registry->create();
			AddComponent<components::TagComponent>(name);
			PWG_INFO("Entity {0} created.", name);
		}
		~Entity() = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args);

		template<typename T>
		T& GetComponent();

		template<typename T>
		bool HasComponent();

		template<typename T>
		void RemoveComponent();
		
	private:
		entt::entity m_entityHandle;
		entt::registry* m_registry;
		std::string m_name;
	};
}

#include "entity.inl"
#endif // !SRC_CORE_ECS_ENTITY_H
