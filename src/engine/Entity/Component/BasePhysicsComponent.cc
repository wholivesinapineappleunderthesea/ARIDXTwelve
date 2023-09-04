#include "BasePhysicsComponent.h"

#include "../Entity.h"

auto BasePhysicsComponent::GetLocalAABB(Math::Vec3& min, Math::Vec3& max) const
	-> void
{
	min = m_localMinOffsetAABB;
	max = m_localMaxOffsetAABB;
}

auto BasePhysicsComponent::GetWorldAABB(Math::Vec3& min, Math::Vec3& max) const
	-> void
{
	const auto lmin = m_localMinOffsetAABB;
	const auto lmax = m_localMaxOffsetAABB;

	const auto ent = GetEntity();
	const auto worldpos = ent->GetWorldPosition();
	const auto worldscale = ent->GetWorldScale();

	min = lmin * worldscale + worldpos;
	max = lmax * worldscale + worldpos;
}

auto BasePhysicsComponent::SetMinAABBOffset(const Math::Vec3& offset) -> void
{
	m_localMinOffsetAABB = offset;
}

auto BasePhysicsComponent::SetMaxAABBOffset(const Math::Vec3& offset) -> void
{
	m_localMaxOffsetAABB = offset;
}

auto BasePhysicsComponent::ApplyForce(const Math::Vec3& force) -> void
{
	// Applies a force to this object's acceleration
	m_acceleration += force / GetMass();
}
