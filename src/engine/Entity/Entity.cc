#include "Entity.h"
#include "../World/World.h"
#include "Component/Component.h"

Entity::Entity() {
	m_components.Reserve(16);
}

Entity::~Entity()
{
	// loop m_components, delete and erase each element
	for (auto it = m_components.begin(); it != m_components.end();)
	{
		const auto c = *it;
		it = m_components.Erase(it);
		delete c;
	}

	// remove from the world
	DetachFromWorld();
}

auto Entity::Update([[maybe_unused]] float deltaTime) -> void
{
	for (auto i = 0; i < m_components.Size(); i++)
	{
		const auto c = m_components[i];
		c->Update(GetWorld(), deltaTime);
	}
}

auto Entity::SetPosition(const Math::Vec3& worldPosition, bool updateChildren)
	-> void
{
	auto worldPos = worldPosition;
	m_position = worldPos;
	if (GetParent())
	{
		const auto parentPosition = GetParent()->GetWorldPosition();
		const auto newPos = worldPos - parentPosition;
		m_position = newPos;
	}
	UpdateWorldMatrix(worldPos, GetWorldScale(), GetWorldRotation());
	if (updateChildren)
	{
		for (auto c : m_children)
		{
			c->SetPosition(c->GetPosition() + worldPos, true);
		}
	}
}

auto Entity::SetScale(const Math::Vec3& worldScale, bool updateChildren) -> void
{
	auto wscale = worldScale;
	m_scale = wscale;
	if (GetParent())
	{
		const auto parentScale = GetParent()->GetWorldScale();
		const auto newScale = wscale / parentScale;
		m_scale = newScale;
	}
	UpdateWorldMatrix(GetWorldPosition(), wscale, GetWorldRotation());
	if (updateChildren)
	{
		for (auto c : m_children)
		{
			c->SetScale(c->GetScale() + wscale, true);
		}
	}
}

inline auto Entity::SetRotation(const Math::Quat& worldRotation,
								bool updateChildren) -> void
{
	auto wrot = worldRotation;
	m_rotation = wrot;
	if (GetParent())
	{
		const auto parentRotation = GetParent()->GetWorldRotation();
		const auto newRot = wrot / parentRotation;
		m_rotation = newRot;
	}
	UpdateWorldMatrix(GetWorldPosition(), GetWorldScale(), wrot);
	if (updateChildren)
	{
		for (auto c : m_children)
		{
			c->SetRotation(c->GetRotation() * wrot, true);
		}
	}
}

auto Entity::AttachToWorld(World* world) -> void
{
	if (world)
	{
		m_world = world;
		m_world->AddEntity(this);
		m_flags.m_isInWorld = true;
	}
	else
	{
		DetachFromWorld();
	}
}
auto Entity::DetachFromWorld() -> void
{
	if (m_world)
	{
		m_world->RemoveEntity(this);
	}
	m_world = nullptr;
	m_flags.m_isInWorld = false;

	RemoveChildren();
}

auto Entity::UpdateWorldMatrix(const Math::Vec3& worldPos,
							   const Math::Vec3& worldScale,
							   const Math::Quat& worldRot) -> void
{
	m_worldPosition = worldPos;
	m_worldScale = worldScale;
	m_worldRotation = worldRot;
	auto mat = Math::Mat4x4::Identity();
	mat = Math::Translate(mat, worldPos);
	mat = Math::Rotate(mat, worldRot);
	mat = Math::Scale(mat, worldScale);
	m_worldMatrix = mat.Transposed();
}

auto Entity::SetParent(Entity* parent) -> void
{
	const auto wpos = GetWorldPosition();
	const auto wscale = GetWorldScale();
	const auto wrot = GetWorldRotation();
	m_parent = parent;
	if (m_parent)
	{
		m_parent->AddChild(this);
	}
	else
	{
		if (m_parent)
		{
			m_parent->RemoveChild(this);
		}
	}
	const auto hasChildren = !m_children.empty();
	SetPosition(wpos, hasChildren);
	SetScale(wscale, hasChildren);
	SetRotation(wrot, hasChildren);
}

auto Entity::AddChild(Entity* child) -> void
{
	m_children.Insert(child);
}
auto Entity::RemoveChild(Entity* child) -> void
{
	for (auto it = m_children.begin(); it != m_children.end();)
	{
		if (*it == child)
		{
			it = m_children.Erase(it);
		}
		else
		{
			it++;
		}
	}
}
auto Entity::RemoveChildren() -> void
{
	for (auto it = m_children.begin(); it != m_children.end();)
	{
		const auto c = *it;
		it = m_children.Erase(it);
		c->SetParent(nullptr);
	}
}