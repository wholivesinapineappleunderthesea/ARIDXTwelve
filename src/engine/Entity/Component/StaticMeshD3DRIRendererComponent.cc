#include "StaticMeshD3DRIRendererComponent.h"
#include "../Entity.h"

#include "../../renderer/D3DRI.h"
#include "../../renderer/D3DRIResourceManager.h"

static auto DestroyResource(D3DRIResource* res) -> void
{
	const auto ri = g_D3DRI;
	const auto rm = ri->GetResourceManager();
	rm->DestroyResource(res);
}

StaticMeshD3DRIRendererComponent::~StaticMeshD3DRIRendererComponent()
{
	SetVertexBuffer(nullptr, false);
	SetIndexBuffer(nullptr, false);
	SetTexture(nullptr, false);
}

auto StaticMeshD3DRIRendererComponent::RenderContext(
	D3DRIRenderFrameContext& context) -> void
{
	(void)context;
	const auto vb = GetVertexBuffer();
	const auto ib = GetIndexBuffer();
	//const auto tex = GetTexture();

	if (!vb)
		return;

	const auto vbres = vb->GetGPUResource();
	const auto vbview = vb->GetGPUVertexBufferView();

	if (!vbres || !vbview.BufferLocation || !vbview.SizeInBytes ||
		!vbview.StrideInBytes)
		return;

	const auto ent = GetEntity();
	const auto& transform = ent->GetWorldMatrix();

	D3DRIRenderFrameContext::Model32BitConstants constants{};
	atd::memcpy(constants.m_modelMatrix, &transform[0][0],
				sizeof(constants.m_modelMatrix));
	constants.m_uvOffset[0] = m_uvOffset[0];
	constants.m_uvOffset[1] = m_uvOffset[1];
	constants.m_uvScale[0] = m_uvScale[0];
	constants.m_uvScale[1] = m_uvScale[1];

	context.SetModelConstants(constants);
	context.SubmitModelConstants();

	context.m_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context.m_list->IASetVertexBuffers(0, 1, &vbview);
	if (ib)
	{
		const auto ibres = ib->GetGPUResource();
		const auto ibview = ib->GetGPUIndexBufferView();
		const auto ibelementsize = static_cast<ULONG>(ib->GetBytesPerElement());
		if (ibres && ibview.BufferLocation && ibview.SizeInBytes &&
			ibelementsize)
		{
			context.m_list->IASetIndexBuffer(&ibview);
			context.m_list->DrawIndexedInstanced(
				ibview.SizeInBytes / ibelementsize, 1, 0, 0, 0);
		}
	}
	else
	{
		context.m_list->DrawInstanced(vbview.SizeInBytes / vbview.StrideInBytes,
									  1, 0, 0);
	}
}

auto StaticMeshD3DRIRendererComponent::SetVertexBuffer(
	D3DRIVertexBufferResource* vertexBuffer, bool owns) -> void
{
	if (m_vertexBuffer && m_ownsVertexBuffer)
	{
		DestroyResource(m_vertexBuffer);
	}
	m_vertexBuffer = vertexBuffer;
	m_ownsVertexBuffer = owns;
	m_genericVertexBuffer = GenericVertexBuffers::GenericVertexBuffers_None;
}

auto StaticMeshD3DRIRendererComponent::SetIndexBuffer(
	D3DRIIndexBufferResource* indexBuffer, bool owns) -> void
{
	if (m_indexBuffer && m_ownsIndexBuffer)
	{
		DestroyResource(m_indexBuffer);
	}
	m_indexBuffer = indexBuffer;
	m_ownsIndexBuffer = owns;
	m_genericTexture = GenericTextures::GenericTextures_None;
}

auto StaticMeshD3DRIRendererComponent::SetTexture(D3DRITextureResource* texture,
												  bool owns) -> void
{
	if (m_texture && m_ownsTexture)
	{
		DestroyResource(m_texture);
	}
	m_texture = texture;
	m_ownsTexture = owns;
	m_genericTexture = GenericTextures::GenericTextures_None;
}

auto StaticMeshD3DRIRendererComponent::SetGenericVertexBuffer(
	GenericVertexBuffers mesh) -> void
{
	SetVertexBuffer(nullptr, false);
	m_genericVertexBuffer = mesh;
}

auto StaticMeshD3DRIRendererComponent::SetGenericIndexBuffer(
	GenericIndexBuffers indexBuffer) -> void
{
	SetIndexBuffer(nullptr, false);
	m_genericIndexBuffer = indexBuffer;
}

auto StaticMeshD3DRIRendererComponent::SetGenericTexture(
	GenericTextures texture) -> void
{
	SetTexture(nullptr, false);
	m_genericTexture = texture;
}

auto StaticMeshD3DRIRendererComponent::GetVertexBuffer()
	-> D3DRIVertexBufferResource*
{
	if (m_vertexBuffer)
		return m_vertexBuffer;
	const auto ri = g_D3DRI;
	D3DRIVertexBufferResource* res{};
	switch (m_genericVertexBuffer)
	{
	case GenericVertexBuffers::GenericVertexBuffers_None:
		break;
	case GenericVertexBuffers::GenericVertexBuffers_Cube:
		res = ri->m_genericCubeResource;
		break;
	default:
		break;
	};
	return res;
}

auto StaticMeshD3DRIRendererComponent::GetIndexBuffer()
	-> D3DRIIndexBufferResource*
{
	if (m_indexBuffer)
		return m_indexBuffer;
	D3DRIIndexBufferResource* res{};
	switch (m_genericIndexBuffer)
	{
	case GenericIndexBuffers::GenericIndexBuffers_None:
		break;
	case GenericIndexBuffers::GenericIndexBuffers_Cube:
		// res = g_D3DRI->m_genericCubeIndexResource;
		break;
	default:
		break;
	};
	return res;
}

auto StaticMeshD3DRIRendererComponent::GetTexture() -> D3DRITextureResource*
{
	if (m_texture)
		return m_texture;

	D3DRITextureResource* res{};
	switch (m_genericTexture)
	{
	case GenericTextures::GenericTextures_None:
		break;
	default:
		break;
	};
	return res;
}
