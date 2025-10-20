#ifndef SRC_CORE_ECS_ENTITY_INL
#define SRC_CORE_ECS_ENTITY_INL

#include "core/logger/logger.h"

namespace pwg
{
	template<typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		PWG_DEBUG("Added {0} to entity {1}.", T::Name, m_name);
		return m_registry->emplace<T>(m_entityHandle, std::forward<Args>(args)...);
	}

	template<typename T>
	T& Entity::GetComponent()
	{
		return m_registry->get<T>(m_entityHandle);
	}

	template<typename T>
	bool Entity::HasComponent()
	{
		return m_registry->any_of<T>(m_entityHandle);
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		m_registry->remove<T>(m_entityHandle);
		PWG_DEBUG("Removed {0} from entity {1}.", T::Name, m_name);
	}
}

#endif // !SRC_CORE_ECS_ENTITY_INL



