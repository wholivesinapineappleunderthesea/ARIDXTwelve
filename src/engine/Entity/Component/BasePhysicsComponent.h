#pragma once

#include "Component.h"

struct BasePhysicsComponent;

enum class CollisionLayerMask : atd::uint32_t
{
	CollisionLayer_None = 0,
	CollisionLayer_WorldStatic = 1 << 1,
	CollisionLayer_WorldDynamic = 1 << 2,
	CollisionLayer_World =
		CollisionLayer_WorldStatic | CollisionLayer_WorldDynamic,

};

struct RayCastHitResult
{
	Math::Vec3 m_hitPosition{};
	Math::Vec3 m_hitNormal{};
	float m_hitDistance{};
	const BasePhysicsComponent* m_hitComponent{};
	const Entity* m_hitEntity{};

	CollisionLayerMask m_hitLayerMask{};
};

struct RayCastQuery
{
	// begin position of the ray cast
	Math::Vec3 m_origin{};
	// end position of the ray cast
	Math::Vec3 m_end{};
	// (end - origin).normalized()
	Math::Vec3 m_direction{};
	// (end - origin).length()
	float m_maxDistance{};
	// layer mask to test against
	CollisionLayerMask m_collisionLayerMask{};
};

struct BasePhysicsComponent : public Component
{
	OBJECTBODYIMPL(BasePhysicsComponent, Component);

	inline auto GetVelocity() const -> const Math::Vec3&
	{
		return m_velocity;
	}
	inline auto SetVelocity(const Math::Vec3& velocity) -> void
	{
		m_velocity = velocity;
	}
	inline auto GetAcceleration() const -> const Math::Vec3&
	{
		return m_acceleration;
	}
	inline auto GetMass() const -> float
	{
		return m_mass;
	}
	inline auto SetMass(float mass) -> void
	{
		m_mass = mass;
	}
	inline auto SetAcceleration(const Math::Vec3& acceleration) -> void
	{
		m_acceleration = acceleration;
	}
	inline auto GetCollisionLayerMask() const -> CollisionLayerMask
	{
		return m_collisionLayerMask;
	}

	virtual auto RayCastTest(RayCastQuery& query, RayCastHitResult& out) const
		-> bool = 0;

	// Gives AABB relative to the entity's position
	auto GetLocalAABB(Math::Vec3& min, Math::Vec3& max) const -> void;
	// Gives AABB relative to the world's position, factors in world scale of
	// the entity
	auto GetWorldAABB(Math::Vec3& min, Math::Vec3& max) const -> void;

	// Set bounding box coordinates, relative to the entity's position
	auto SetMinAABBOffset(const Math::Vec3& offset) -> void;
	auto SetMaxAABBOffset(const Math::Vec3& offset) -> void;

	// Set the collision mask
	inline auto SetCollisionLayerMask(CollisionLayerMask collisionLayer) -> void
	{
		m_collisionLayerMask = collisionLayer;
	}

	// Gravity
	inline auto GetGravityConstant() const -> const Math::Vec3&
	{
		return m_gravityConstant;
	}
	inline auto SetGravityConstant(const Math::Vec3& gravity) -> void
	{
		m_gravityConstant = gravity;
	}
	inline auto GetGravityVelocity() const -> const Math::Vec3& {
		return m_gravityVelocity;
	}
	inline auto SetGravityVelocity(const Math::Vec3& vel) -> void {
		m_gravityVelocity = vel;
		if (m_gravityVelocity.LengthSquared() > GetGravityConstant().LengthSquared())
		{
			m_gravityVelocity = m_gravityVelocity.Normalized() * GetGravityConstant().Length();
		}
	}
	inline auto IsGravityEnabled() const -> bool
	{
		return m_gravityEnabled;
	}
	inline auto SetGravityEnabled(bool enabled) -> void
	{
		m_gravityEnabled = enabled;
	}

	virtual auto ApplyForce(const Math::Vec3& force) -> void;

  protected:
  private:
	Math::Vec3 m_velocity{};
	Math::Vec3 m_acceleration{};
	float m_mass{1.f};
	Math::Vec3 m_gravityConstant{0.f, -9.81f, 0.f};
	Math::Vec3 m_gravityVelocity{};
	CollisionLayerMask m_collisionLayerMask{};
	Math::Vec3 m_localMinOffsetAABB{};
	Math::Vec3 m_localMaxOffsetAABB{};

	int8_t m_gravityEnabled : 1 {1};
};