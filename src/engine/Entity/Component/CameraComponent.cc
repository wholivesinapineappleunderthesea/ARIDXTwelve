#include "CameraComponent.h"
#include "../Entity.h"
#include "../../GlobalWindowData.h"

CameraComponent::CameraComponent()
{
	UpdatePitchYawRollFromAxisAngles();
	UpdateProjectionMatrix();
	UpdateViewMatrix();
	UpdateViewProjectionMatrix();
}

CameraComponent::~CameraComponent()
{
}

auto CameraComponent::Update([[maybe_unused]] World* world,
							 [[maybe_unused]] float deltaTime) -> void
{
	m_aspectRatio = g_windowData->m_aspectRatio;
	m_fovy = m_fovx / m_aspectRatio;


	if (GetShouldUpdateFromEntityPosition())
	{
		const auto ent = GetEntity();
		const auto wpos = ent->GetWorldPosition();
		m_camWorldPosition = wpos + m_positionOffset;
	}
	if (IsInFreeCamMode())
	{
		const auto fkey = g_windowData->m_keyStates[VK_UP];
		const auto bkey = g_windowData->m_keyStates[VK_DOWN];
		const auto lkey = g_windowData->m_keyStates[VK_LEFT];
		const auto rkey = g_windowData->m_keyStates[VK_RIGHT];
		const auto speed = GetFreeCamModeSpeed() * deltaTime;
		if (fkey)
		{
			SetWorldPosition(GetWorldPosition() + GetForward() * speed);
		}
		if (bkey)
		{
			SetWorldPosition(GetWorldPosition() - GetForward() * speed);
		}
		if (lkey)
		{
			SetWorldPosition(GetWorldPosition() - GetRight() * speed);
		}
		if (rkey)
		{
			SetWorldPosition(GetWorldPosition() + GetRight() * speed);
		}
	}

	UpdateProjectionMatrix();
	UpdateViewMatrix();
	UpdateViewProjectionMatrix();
}

auto CameraComponent::SetCameraEntityOffset(const Math::Vec3& offset) -> void {
	m_positionOffset = offset;
	const auto ent = GetEntity();
	const auto wpos = ent->GetWorldPosition();
	m_camWorldPosition = wpos + m_positionOffset;
	UpdateViewMatrix();
	UpdateViewProjectionMatrix();
}

auto CameraComponent::ApplyMouseMovement(atd::int32_t mouseX,
										 atd::int32_t mouseY) -> void
{
	const auto xoffset = static_cast<float>(mouseX) * m_mouseSensitivity;
	const auto yoffset = static_cast<float>(-mouseY) * m_mouseSensitivity;

	m_yaw += xoffset;
	m_pitch += yoffset;

	ClampPitchYawRoll();
	UpdateAxisAnglesFromPitchYawRoll();
	UpdateProjectionMatrix();
	UpdateViewMatrix();
	UpdateViewProjectionMatrix();
}
