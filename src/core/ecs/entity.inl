#ifndef SRC_CORE_ECS_ENTITY_INL
#define SRC_CORE_ECS_ENTITY_INL

namespace pwg
{
	template<typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
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
	}
}

#endif // !SRC_CORE_ECS_ENTITY_INL



