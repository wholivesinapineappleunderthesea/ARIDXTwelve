#pragma once

#include "Component.h"

struct CameraComponent : public Component
{
	OBJECTBODYIMPL(CameraComponent, Component);

	CameraComponent();
	~CameraComponent() override;

	auto Update([[maybe_unused]] World* world, [[maybe_unused]] float deltaTime)
		-> void override;

	inline auto GetProjectionMatrix() const -> Math::Mat4x4
	{
		return m_projectionMatrix;
	}
	inline auto GetViewMatrix() const -> Math::Mat4x4
	{
		return m_viewMatrix;
	}
	inline auto GetViewProjectionMatrix() const -> Math::Mat4x4
	{
		return m_viewProjectionMatrix;
	}

	inline auto GetWorldPosition() const -> Math::Vec3
	{
		return m_camWorldPosition;
	}
	inline auto SetWorldPosition(const Math::Vec3& p) -> void
	{
		m_camWorldPosition = p;
		UpdateViewMatrix();
		UpdateViewProjectionMatrix();
	}
	inline auto GetCameraEntityOffset() const -> Math::Vec3
	{
		return m_positionOffset;
	}
	auto SetCameraEntityOffset(const Math::Vec3& offset) -> void;

	inline auto GetForward() const -> Math::Vec3
	{
		return m_forward;
	}
	inline auto GetUp() const -> Math::Vec3
	{
		return m_up;
	}
	inline auto GetRight() const -> Math::Vec3
	{
		return m_right;
	}

	inline auto LookAt(const Math::Vec3& position) -> void
	{
		m_forward = (position - m_camWorldPosition).Normalized();
		m_right = Math::Vec3::Up().Cross(m_forward).Normalized();
		m_up = m_forward.Cross(m_right).Normalized();
		UpdateProjectionMatrix();
		UpdateViewMatrix();
		UpdateViewProjectionMatrix();
		UpdatePitchYawRollFromAxisAngles();
	}

	inline auto SetFOV(float horiz) -> void
	{
		m_fovx = horiz;
		m_fovy = horiz / m_aspectRatio;
		UpdateProjectionMatrix();
		UpdateViewProjectionMatrix();
	}

	inline auto CanMouseLook() const -> bool
	{
		return m_canMouseLook;
	}
	inline auto SetCanMouseLook(bool canMouseLook) -> void
	{
		m_canMouseLook = canMouseLook;
	}
	inline auto GetMouseSensitivity() const -> float
	{
		return m_mouseSensitivity;
	}
	inline auto SetMouseSensitivity(float mouseSensitivity) -> void
	{
		m_mouseSensitivity = mouseSensitivity;
	}
	inline auto IsInFreeCamMode() const -> bool
	{
		return m_InFreeCamMode;
	}
	inline auto SetInFreeCamMode(bool freeCam) -> void
	{
		m_InFreeCamMode = freeCam;
		SetShouldUpdateFromEntityPosition(!freeCam);
	}
	inline auto GetFreeCamModeSpeed() const -> float
	{
		return m_freeCamSpeed;
	}
	inline auto SetFreeCamModeSpeed(float freeCamSpeed) -> void
	{
		m_freeCamSpeed = freeCamSpeed;
	}
	inline auto GetShouldUpdateFromEntityPosition() const -> bool
	{
		return m_shouldUpdateFromEntityPosition;
	}
	inline auto
	SetShouldUpdateFromEntityPosition(bool shouldUpdateFromEntityPosition)
		-> void
	{
		m_shouldUpdateFromEntityPosition = shouldUpdateFromEntityPosition;
	}

	auto ApplyMouseMovement(atd::int32_t mouseX, atd::int32_t mouseY) -> void;

  private:
	// Camera matrices
	Math::Mat4x4 m_viewMatrix{};
	Math::Mat4x4 m_projectionMatrix{};
	Math::Mat4x4 m_viewProjectionMatrix{};
	inline auto UpdateProjectionMatrix() -> void
	{
		m_projectionMatrix = Math::Mat4x4::PerspectiveRH_ZO(
								 Math::FDegreesToRadians(m_fovy), m_aspectRatio,
								 m_nearPlane, m_farPlane)
								 .Transposed();
	}
	inline auto UpdateViewMatrix() -> void
	{
		m_viewMatrix =
			Math::Mat4x4::LookAtRH(m_camWorldPosition,
								   m_camWorldPosition + m_forward, m_up)
				.Transposed();
	}
	inline auto UpdateViewProjectionMatrix() -> void
	{
		m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	}

	// World position of the camera
	Math::Vec3 m_camWorldPosition{};
	// Camera offset from entity position
	Math::Vec3 m_positionOffset{};

	// The camera's local axes
	Math::Vec3 m_forward{Math::Vec3::Forward()};
	Math::Vec3 m_up{Math::Vec3::Up()};
	Math::Vec3 m_right{Math::Vec3::Right()};
	inline auto UpdateAxisAnglesFromPitchYawRoll() -> void
	{
		m_forward.x = Math::FCos(Math::FDegreesToRadians(m_yaw)) *
					  Math::FCos(Math::FDegreesToRadians(m_pitch));
		m_forward.y = Math::FSin(Math::FDegreesToRadians(m_pitch));
		m_forward.z = Math::FSin(Math::FDegreesToRadians(m_yaw)) *
					  Math::FCos(Math::FDegreesToRadians(m_pitch));
		m_forward.Normalize();

		m_right = m_forward.Cross(Math::Vec3::Up()).Normalized();
		m_up = m_right.Cross(m_forward).Normalized();
	}

	// Pitch and yaw in degrees
	float m_pitch{};
	float m_yaw{};
	float m_roll{};
	inline auto UpdatePitchYawRollFromAxisAngles() -> void
	{
		m_pitch = Math::FRadiansToDegrees(Math::FAsin(m_forward.y));
		m_yaw = Math::FRadiansToDegrees(Math::FAtan2(m_forward.x, m_forward.z));
		m_roll = Math::FRadiansToDegrees(Math::FAtan2(m_right.y, m_up.y));
	}
	inline auto ClampPitchYawRoll() -> void
	{
		if (m_pitch > 89.f)
		{
			m_pitch = 89.f;
		}
		else if (m_pitch < -89.f)
		{
			m_pitch = -89.f;
		}
		while (m_yaw > 360.f)
		{
			m_yaw -= 360.f;
		}
		while (m_yaw < 0.f)
		{
			m_yaw += 360.f;
		}
		while (m_roll > 360.f)
		{
			m_roll -= 360.f;
		}
		while (m_roll < 0.f)
		{
			m_roll += 360.f;
		}
	}

	// Aspect ratio of the viewport
	float m_aspectRatio{16.f / 9.f};

	// FOV in degrees
	float m_fovx{160.f};
	float m_fovy{160.f / (16.f / 9.f)};

	// Clipping
	float m_nearPlane{0.01f};
	float m_farPlane{1000.f};

	// Mouse movement
	float m_mouseSensitivity{0.12f};

	// Freecam
	float m_freeCamSpeed{2.f};

	// Flags:
	atd::int8_t m_canMouseLook : 1 {0};
	// If set, Update() will look at direction keys to move the camera. Speed
	// set by m_freeCamSpeed.
	atd::int8_t m_InFreeCamMode : 1 {0};
	// If the camera should update from the entity's world position and the
	// camera offset
	atd::int8_t m_shouldUpdateFromEntityPosition : 1 {0};
};