#ifndef SRC_CORE_ECS_ENTITY_H
#define SRC_CORE_ECS_ENTITY_H

#include <entt/entt.hpp>

namespace pwg
{
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity entityHandle, entt::registry* registry) : m_entityHandle(entityHandle), m_registry(registry) {}
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
	};
}

#include "entity.inl"
#endif // !SRC_CORE_ECS_ENTITY_H
