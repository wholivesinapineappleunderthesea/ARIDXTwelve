#pragma once

#include "BasePhysicsComponent.h"

struct BoundingBoxComponent : public BasePhysicsComponent
{
	OBJECTBODYIMPL(BoundingBoxComponent, BasePhysicsComponent);

	auto Update([[maybe_unused]] World* world, [[maybe_unused]] float deltaTime)
		-> void override;

	auto RayCastTest(RayCastQuery& query, RayCastHitResult& out) const
		-> bool override;

};