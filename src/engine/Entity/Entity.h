#pragma once

#include "../Object.h"

struct World;
struct Component;

struct Entity : public Object
{
	OBJECTBODYIMPL(Entity, Object);
	Entity();
	~Entity() override;

	virtual auto Update(float deltaTime) -> void;

	// Gets the world matrix:
	inline auto GetWorldMatrix() const -> const Math::Mat4x4&
	{
		return m_worldMatrix;
	}

	// Entity position:
	// Will return local position if there is a parent. Returns world position
	// if not.
	inline auto GetPosition() const -> const Math::Vec3&
	{
		return m_position;
	}
	// Gets the world position:
	inline auto GetWorldPosition() const -> Math::Vec3
	{
		return m_worldPosition;
	}
	// Set the position by world position.
	// If there is a parent, it will move the entity relative to the parent.
	// If there is no parent, it will move the entity relative to the world.
	// If updateChildren is true, it will update the children's world position
	// relative to this entity's.
	virtual auto SetPosition(const Math::Vec3& worldPosition,
							 bool updateChildren) -> void;

	// Entity scale:
	// Will return local scale if there is a parent. Returns world scale if not.
	inline auto GetScale() const -> const Math::Vec3&
	{
		return m_scale;
	}
	// Gets the world scale:
	inline auto GetWorldScale() const -> Math::Vec3
	{
		return m_worldScale;
	}
	// Set the scale by world scale.
	// If there is a parent, it will scale the entity relative to the parent.
	// If there is no parent, it will scale the entity relative to the world.
	// If updateChildren is true, it will update the children's world scale
	// relative to this entity's.
	auto SetScale(const Math::Vec3& worldScale, bool updateChildren) -> void;

	// Entity rotation:
	// Will return local rotation if there is a parent. Returns world rotation
	// if not.
	inline auto GetRotation() const -> const Math::Quat&
	{
		return m_rotation;
	}
	// Gets the world rotation:
	inline auto GetWorldRotation() const -> Math::Quat
	{
		return m_worldRotation;
	}
	// Set the rotation by world rotation.
	// If there is a parent, it will rotate the entity relative to the parent.
	// If there is no parent, it will rotate the entity relative to the world.
	// If updateChildren is true, it will update the children's world rotation
	// relative to this entity's.
	inline auto SetRotation(const Math::Quat& worldRotation,
							bool updateChildren) -> void;

	inline auto GetParent() const -> Entity*
	{
		return m_parent;
	}
	auto SetParent(Entity* parent) -> void;
	inline auto GetWorld() const -> World*
	{
		return m_world;
	}

	template <typename T>
	inline auto GetComponent() -> T*
	requires atd::derived_from<T, Component>
	{
		for (auto& component : m_components)
		{
			if (component->IsA<T>())
			{
				return component->Cast<T>();
			}
		}
		return nullptr;
	}
	template <typename T>
	inline auto GetComponent() const -> const T*
	requires atd::derived_from<T, Component>
	{
		for (auto& component : m_components)
		{
			if (component->IsA<T>())
			{
				return component->Cast<const T>();
			}
		}
		return nullptr;
	}

	template <typename T>
	inline auto AddComponent() -> T*
		requires atd::derived_from<T, Component>
	{
		const auto comp = new T{};
		comp->SetEntity(this);
		m_components.Insert(comp);
		return comp;
	}
	inline auto CanUpdate() const -> bool
	{
		return m_flags.m_canUpdate;
	}
	inline auto SetCanUpdate(bool canUpdate) -> void
	{
		m_flags.m_canUpdate = canUpdate;
	}
	inline auto IsStatic() const -> bool
	{
		return m_flags.m_isStatic;
	}
	inline auto SetStatic(bool isStatic) -> void
	{
		m_flags.m_isStatic = isStatic;
	}
	inline auto IsInWorld() const -> bool
	{
		return m_flags.m_isInWorld;
	}
	auto AttachToWorld(World* world) -> void;
	auto DetachFromWorld() -> void;
	inline auto IsPhysicsEnabled() const -> bool
	{
		return m_flags.m_isPhysicsEnabled;
	}
	inline auto SetPhysicsEnabled(bool isPhysicsEnabled) -> void
	{
		m_flags.m_isPhysicsEnabled = isPhysicsEnabled;
	}
	inline auto IsRenderable() const -> bool
	{
		return m_flags.m_isRenderingEnabled;
	}
	inline auto SetRenderable(bool isRenderable) -> void
	{
		m_flags.m_isRenderingEnabled = isRenderable;
	}
	inline auto GetName() const -> const char*
	{
		return m_name;
	}
	inline auto SetName(const char* name) -> void
	{
		m_name = name;
	}

  protected:
	// World matrix,
	// updated every time the position, rotation, or scale is set
	auto UpdateWorldMatrix(const Math::Vec3& worldPos,
						   const Math::Vec3& worldScale,
						   const Math::Quat& worldRot) -> void;
	Math::Mat4x4 m_worldMatrix{};
	// World position, scale, and rotation.
	// Updated in UpdateWorldMatrix()
	Math::Vec3 m_worldPosition{0.f, 0.f, 0.f};
	Math::Vec3 m_worldScale{1.f, 1.f, 1.f};
	Math::Quat m_worldRotation{1.f, 0.f, 0.f, 0.f};
	// If we have a parent entity, this is a relative position
	// Use GetWorldPosition() to get the absolute position
	Math::Vec3 m_position{0.f, 0.f, 0.f};
	// If we have a parent entity, this is relative scale
	// Use GetWorldScale() to get the absolute scale
	Math::Vec3 m_scale{1.f, 1.f, 1.f};
	// If we have a parent entity, this is a relative rotation
	// Use GetWorldRotation() to get the absolute rotation
	Math::Quat m_rotation{1.f, 0.f, 0.f, 0.f};
	// If we have a parent entity, this is a relative rotation
	// Use GetWorldRotation() to get the absolute rotation

	Entity* m_parent{};
	Array<Entity*> m_children{};
	auto AddChild(Entity* child) -> void;
	auto RemoveChild(Entity* child) -> void;
	auto RemoveChildren() -> void;
	World* m_world{};
	Array<Component*> m_components{};
	struct EntityFlags
	{
		// If the entity will be updated in Update()
		// Entity::Update calls components' Update functions so those won't be
		// called if this is not set
		atd::uint8_t m_canUpdate : 1;
		// If the entity is a static object (i.e. a wall)
		atd::uint8_t m_isStatic : 1;
		// If the entity has a world set
		atd::uint8_t m_isInWorld : 1;
		// If the entity has physics enabled
		atd::uint8_t m_isPhysicsEnabled : 1;
		// If the entity is renderable
		atd::uint8_t m_isRenderingEnabled : 1;
	} m_flags{};
	const char* m_name{};
};