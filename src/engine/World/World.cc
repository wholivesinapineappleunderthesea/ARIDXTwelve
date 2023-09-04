#include "World.h"
#include "../Entity/Component/CameraComponent.h"
#include "../Entity/Entity.h"

#include "../GlobalWindowData.h"

World::World()
{
	// Register mouse callback
	g_windowData->m_callbacks.Lock();
	const auto cb = [](void* ctx, atd::int32_t x, atd::int32_t y) {
		const auto w = static_cast<World*>(ctx);
		w->Lock();
		w->OnMouseMove(x, y);
		w->Unlock();
	};
	g_windowData->m_callbacks.m_onMouseMove.Insert(
		GlobalWindowCallbackEntry<void, atd::int32_t, atd::int32_t>{this, cb});
	g_windowData->m_callbacks.Unlock();
}

World::~World()
{

	g_windowData->m_callbacks.Lock();
	for (auto& c : g_windowData->m_callbacks.m_onMouseMove)
	{
		if (c.m_ctx == this)
		{
			g_windowData->m_callbacks.m_onMouseMove.Erase(&c);
			break;
		}
	}
	g_windowData->m_callbacks.Unlock();
}

auto World::Update([[maybe_unused]] float deltaTime) -> void
{
	for (auto ent : m_entityList)
	{
		if (ent->CanUpdate())
		{
			ent->Update(deltaTime);
		}
	}
}

auto World::AddEntity(Entity* entity) -> void
{
	m_entityList.Insert(entity);
}

auto World::RemoveEntity(Entity* entity) -> void
{
	for (auto it = m_entityList.begin(); it != m_entityList.end(); ++it)
	{
		if (*it == entity)
		{
			m_entityList.Erase(it);
			break;
		}
	}
}

auto World::OnMouseMove(atd::int32_t x, atd::int32_t y) -> void
{
	(void)x;
	(void)y;
	const auto cam = GetRenderingCameraComponent();
	if (cam)
	{
		if (cam->CanMouseLook())
		{
			cam->ApplyMouseMovement(x, y);
		}
	}
}

auto World::GetRenderingCameraComponent() -> CameraComponent*
{
	const auto lp = GetCurrentLocalPlayerEntity();
	if (lp)
	{
		return lp->GetComponent<CameraComponent>();
	}
	return nullptr;
}
