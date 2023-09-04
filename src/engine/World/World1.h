#pragma once

#include "World.h"

struct World1 : public World
{
	OBJECTBODYIMPL(World1, World);

	World1();
	~World1() override;

	auto Update(float deltaTime) -> void override;

	auto GetRenderingCameraComponent() -> CameraComponent* override;

	auto CreateStaticGeometryEntity() -> void;
	auto CreatePlayerEntity() -> void;

  private:

	// World entities
	Entity* m_defaultPlayerEntity{};
	Entity* m_worldMeshEntity{};
};