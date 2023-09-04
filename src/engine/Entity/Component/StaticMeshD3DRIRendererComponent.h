#pragma once

#include "BaseD3DRIRendererComponent.h"

struct D3DRIVertexBufferResource;
struct D3DRIIndexBufferResource;
struct D3DRITextureResource;

struct StaticMeshD3DRIRendererComponent : public BaseD3DRIRendererComponent
{
	OBJECTBODYIMPL(StaticMeshD3DRIRendererComponent,
				   BaseD3DRIRendererComponent);

	~StaticMeshD3DRIRendererComponent() override;

	auto RenderContext(D3DRIRenderFrameContext& context) -> void override;

	// If owns is set, the destructor will call
	// D3DRIResourceManager::DestroyResource
	auto SetVertexBuffer(D3DRIVertexBufferResource* vertexBuffer, bool owns)
		-> void;
	auto SetIndexBuffer(D3DRIIndexBufferResource* indexBuffer, bool owns)
		-> void;
	auto SetTexture(D3DRITextureResource* texture, bool owns) -> void;

	enum class GenericVertexBuffers
	{
		GenericVertexBuffers_None,
		GenericVertexBuffers_Cube,
		GenericVertexBuffers_MAX,
	};
	enum class GenericIndexBuffers
	{
		GenericIndexBuffers_None,
		GenericIndexBuffers_Cube,
		GenericIndexBuffers_MAX,
	};
	enum class GenericTextures
	{
		GenericTextures_None,
		GenericTextures_MAX,
	};
	// Set from default resources
	auto SetGenericVertexBuffer(GenericVertexBuffers mesh) -> void;
	auto SetGenericIndexBuffer(GenericIndexBuffers indexBuffer) -> void;
	auto SetGenericTexture(GenericTextures texture) -> void;

	auto GetVertexBuffer() -> D3DRIVertexBufferResource*;
	auto GetIndexBuffer() -> D3DRIIndexBufferResource*;
	auto GetTexture() -> D3DRITextureResource*;

	inline auto GetUVScale() -> const float*
	{
		return m_uvScale;
	}
	inline auto GetUVOffset() -> const float*
	{
		return m_uvOffset;
	}

	inline auto SetUVScale(float u, float v) -> void
	{
		m_uvScale[0] = u;
		m_uvScale[1] = v;
	}
	inline auto SetUVOffset(float u, float v) -> void
	{
		m_uvOffset[0] = u;
		m_uvOffset[1] = v;
	}

  private:
	// If using a generic vertex buffer, this will be nullptr. Use GetMesh
	// instead
	D3DRIVertexBufferResource* m_vertexBuffer{};
	bool m_ownsVertexBuffer{};
	GenericVertexBuffers m_genericVertexBuffer{};

	// If using a generic index buffer, this will be nullptr. Use GetIndexBuffer
	// instead
	D3DRIIndexBufferResource* m_indexBuffer{};
	bool m_ownsIndexBuffer{};
	GenericIndexBuffers m_genericIndexBuffer{};

	// If using a generic texture, this will be nullptr. Use GetTexture instead
	D3DRITextureResource* m_texture{};
	bool m_ownsTexture{};
	GenericTextures m_genericTexture{};

	float m_uvScale[2]{1.f, 1.f};
	float m_uvOffset[2]{0.f, 0.f};
};