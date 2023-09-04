#pragma once

#include "../Object.h"

struct Entity;
struct CameraComponent;

struct World : public Object
{
	OBJECTBODYIMPL(World, Object)
	World();
	virtual ~World();

	virtual auto Update(float deltaTime) -> void;

	inline auto GetWorldName() -> const char*
	{
		return m_worldName;
	}

	virtual auto GetRenderingCameraComponent() -> CameraComponent*;

	virtual inline auto GetCurrentLocalPlayerEntity() -> Entity*
	{
		return m_currentLocalPlayerEntity;
	}
	virtual inline auto SetCurrentLocalPlayerEntity(Entity* entity) -> void
	{
		m_currentLocalPlayerEntity = entity;
	}

	inline auto Lock() -> void
	{
		m_worldMutex.lock();
	}
	inline auto Unlock() -> void
	{
		m_worldMutex.unlock();
	}

  protected:
	virtual auto OnMouseMove(atd::int32_t x, atd::int32_t y) -> void;

	// Called by Entity::SetWorld
	virtual auto AddEntity(Entity* entity) -> void;
	virtual auto RemoveEntity(Entity* entity) -> void;
	friend struct Entity;

	// Call in the subclass constructor
	inline auto SetWorldName(const char* name) -> void
	{
		m_worldName = name;
	}

  public:
	Array<Entity*> m_entityList{};

  private:
	const char* m_worldName{};

	// Active local player entity
	Entity* m_currentLocalPlayerEntity{};

	// World mutex
	Mutex m_worldMutex{};
};
