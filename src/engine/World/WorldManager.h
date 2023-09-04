#pragma once

#include "World.h"

struct WorldManager
{
	inline WorldManager() = default;
	inline ~WorldManager() {
		Lock();
		delete m_world;
		Unlock();
	}

	inline auto GetWorld() -> World*
	{
		return m_world;
	}

	template <typename T> inline auto LoadWorld() -> void
	{
		if (m_world)
		{
			delete m_world;
		}

		m_world = new T();
	}

	// Update thread methods:
	inline auto Update([[maybe_unused]] float dt) -> void
	{
		const auto w = GetWorld();
		if (w)
		{
			w->Lock();
			w->Update(dt);
			w->Unlock();
		}
	}

	// The renderer and update thread both need to lock this before accessing
	// the world pointer in world manager
	inline auto Lock() -> void
	{
		m_worldMutex.lock();
	}
	inline auto Unlock() -> void
	{
		m_worldMutex.unlock();
	}

  private:
	World* m_world{};
	Mutex m_worldMutex{};
};

inline WorldManager* g_worldManager{};