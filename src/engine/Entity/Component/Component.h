#pragma once

#include "../../Object.h"

struct Entity;
struct World;

struct Component : public Object
{
	OBJECTBODYIMPL(Component, Object);

	virtual inline auto Update([[maybe_unused]] World* world,
							 [[maybe_unused]] float deltaTime) -> void
	{
	}

	inline auto GetEntity() const -> Entity*
	{
		return m_entity;
	}

  private:
	inline auto SetEntity(Entity* e)
	{
		m_entity = e;
	}
	Entity* m_entity{};
	friend struct Entity;
};