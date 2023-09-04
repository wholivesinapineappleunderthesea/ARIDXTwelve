#pragma once

#include "Component.h"

struct D3DRIRenderFrameContext;

struct BaseD3DRIRendererComponent : public Component
{
	OBJECTBODYIMPL(BaseD3DRIRendererComponent, Component);

	virtual auto RenderContext(D3DRIRenderFrameContext& context) -> void = 0;
};