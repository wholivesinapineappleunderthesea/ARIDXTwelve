#include "BoundingBoxComponent.h"
#include "../Entity.h"
auto BoundingBoxComponent::Update([[maybe_unused]] World* world,
								  [[maybe_unused]] float deltaTime) -> void
{
	const auto entity = GetEntity();
	if (!entity)
		return;
	if (!entity->IsPhysicsEnabled())
		return;
	
	const auto worldPosition = entity->GetWorldPosition();
	// const auto worldScale = entity->GetWorldScale();

	auto velocity = GetVelocity();
	const auto accel = GetAcceleration();
	SetVelocity(velocity + accel);

	velocity += accel * deltaTime;

	if (IsGravityEnabled())
	{
		const auto gravConstant = GetGravityConstant();
		const auto gvel = GetGravityVelocity();
		velocity += gvel;
		SetGravityVelocity(gvel + gravConstant * deltaTime);
	}

	const auto newWorldPosition = worldPosition + velocity * deltaTime;

	entity->SetPosition(newWorldPosition, true);
}

auto BoundingBoxComponent::RayCastTest(RayCastQuery& query,
									   RayCastHitResult& out) const -> bool
{
	(void)out;
	const auto entity = GetEntity();
	if (!entity)
		return false;
	if (!entity->IsPhysicsEnabled())
		return false;

	const auto rayBegin = query.m_origin;
	const auto rayEnd = query.m_end;

	Math::Vec3 worldMinAABB{}, worldMaxAABB{};
	GetWorldAABB(worldMinAABB, worldMaxAABB);

	// Test if the ray passes through our AABB, if not there's no point to do
	// any further test
	if (!Math::DoesLineSegmentIntersectWithAABB(rayBegin, rayEnd, worldMinAABB,
												worldMaxAABB))
		return false;

	// Find the intersection point
	Math::Vec3 intersectionPoint{};
	Math::Vec3 intersectionNormal{};
	float intersectionDistance{};
	if (Math::FindLineSegmentIntersectionWithAABB(
			rayBegin, rayEnd, worldMinAABB, worldMaxAABB, intersectionPoint,
			intersectionDistance, intersectionNormal))
	{

		out.m_hitPosition = intersectionPoint;
		out.m_hitNormal = intersectionNormal;
		out.m_hitDistance = intersectionDistance;
		out.m_hitComponent = this;
		out.m_hitEntity = entity;
		out.m_hitLayerMask = GetCollisionLayerMask();

		return true;
	}

	return false;
}