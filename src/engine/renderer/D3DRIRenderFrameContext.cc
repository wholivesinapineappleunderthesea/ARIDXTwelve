#include "../Entity/Component/BaseD3DRIRendererComponent.h"
#include "../Entity/Component/CameraComponent.h"
#include "../Entity/Entity.h"
#include "../GlobalWindowData.h"
#include "../World/WorldManager.h"
#include "D3DRI.h"

auto D3DRIRenderFrameContext::SubmitCameraConstants() -> void
{
	m_list->SetGraphicsRoot32BitConstants(0, sizeof m_cameraConstants / 4,
										  &m_cameraConstants, 0);
}

auto D3DRIRenderFrameContext::SubmitModelConstants() -> void
{
	m_list->SetGraphicsRoot32BitConstants(1, sizeof m_modelConstants / 4,
										  &m_modelConstants, 0);
}

auto D3DRIRenderFrameContext::FillCommandList() -> void
{
	Camera32BitConstants camConstants{};
	// Set camera viewport and time from ri
	camConstants.m_viewportWidth = m_ri->m_viewport.Width;
	camConstants.m_viewportHeight = m_ri->m_viewport.Height;
	camConstants.m_time = m_ri->m_time;

	if (g_worldManager)
	{
		g_worldManager->Lock();
		const auto world = g_worldManager->GetWorld();
		if (world)
		{
			world->Lock();
			// Get camera from world
			const auto cam = world->GetRenderingCameraComponent();
			if (cam)
			{
				const auto proj = cam->GetProjectionMatrix();
				const auto view = cam->GetViewMatrix();

				// Set camera constants
				atd::memcpy(&camConstants.m_projectionMatrix, &proj,
					sizeof camConstants.m_projectionMatrix);
				atd::memcpy(&camConstants.m_viewMatrix, &view,
					sizeof camConstants.m_viewMatrix);
				// Submit the world's camera constants here
				SetCameraConstants(camConstants);
				SubmitCameraConstants();
			}
			

			// Iterate all renderable entities and render them
			for (auto e : world->m_entityList)
			{
				if (e->IsRenderable())
				{
					const auto comp =
						e->GetComponent<BaseD3DRIRendererComponent>();
					if (comp)
					{
						// Render entity
						comp->RenderContext(*this);
					}
				}
			}
			world->Unlock();
		}

		g_worldManager->Unlock();
	}
}
