#include "D3DRIResource.h"

#include "../Common.h"

D3DRIResource::D3DRIResource(D3DRIResourceType type, const atd::uint8_t* data,
							 atd::size_t sizeBytes, atd::size_t strideBytes)
{
	m_type = type;
	D3DRIASSERT(data);
	D3DRIASSERT(sizeBytes > 0);

	const auto p = new atd::uint8_t[sizeBytes];
	D3DRIASSERT(p);
	atd::memcpy(p, data, sizeBytes);
	m_rawData = p;
	m_rawDataSizeInBytes = sizeBytes;
	m_rawDataStrideInBytes = strideBytes;
}

D3DRIResource::~D3DRIResource()
{
	delete m_rawData;
}

auto D3DRIResource::GetType() const -> D3DRIResourceType
{
	return m_type;
}

auto D3DRITextureResource::CreateGPUResource(
	D3DRIBatchResourceCreationContext& ctx) -> void
{
	const auto srvHeap = ctx.m_ri->m_d3dSRVDescriptorHeap;
	D3DRIASSERT(srvHeap);

	const auto width = m_width;
	const auto height = m_height;
	const auto depth = m_depth;
	const auto mipLevels = m_mipLevels;

	// Create the texture resource:
	D3D12_RESOURCE_DESC textureDesc{};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = static_cast<UINT16>(depth); // 1
	textureDesc.MipLevels = static_cast<UINT16>(mipLevels);	   // 1
	textureDesc.Format = m_format; // DXGI_FORMAT_R8G8B8A8_UNORM
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	m_gpuTextureResource = ctx.m_ri->CreateBufferResource(
		textureDesc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);

	// Create the shader resource view:
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	ctx.m_device->CreateShaderResourceView(
		GetGPUResource(), &srvDesc,
		srvHeap->GetCPUDescriptorHandleForHeapStart());

	// Create the upload buffer:
	// TODO: Reuse same code or move into D3DRI.cc.
	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	const auto uploadpitch =
		(width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) &
		~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
	const auto uploadsize = height * uploadpitch;
	bufferDesc.Width = uploadsize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	m_gpuUploadBufferResource = ctx.m_ri->CreateBufferResource(bufferDesc);
	D3DRIASSERT(m_gpuUploadBufferResource);
	m_gpuUploadBufferResource->SetName(
		L"D3DRITextureResource::m_gpuUploadBufferResource");

	// Write to the upload buffer:
	void* mapped{};
	D3D12_RANGE range{0, 0};
	auto hr = m_gpuUploadBufferResource->Map(0, &range, &mapped);
	if (D3DRI::IsError(hr, "Map"))
	{
		ctx.m_ri->FatalError("Map failed");
	}
	for (unsigned int y{0}; y < height; y++)
	{
		const auto dst = reinterpret_cast<void*>(
			reinterpret_cast<atd::uintptr_t>(mapped) + y * uploadpitch);
		const auto src = reinterpret_cast<void*>(
			reinterpret_cast<atd::uintptr_t>(m_rawData) + y * width * 4);
		atd::memcpy(dst, src, width * 4);
	}
	m_gpuUploadBufferResource->Unmap(0, &range);

	// Copy from the upload buffer to the texture:
	D3D12_TEXTURE_COPY_LOCATION srcLocation{};
	srcLocation.pResource = m_gpuUploadBufferResource.Get();
	srcLocation.Type = ::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLocation.PlacedFootprint.Footprint.Format = ::DXGI_FORMAT_R8G8B8A8_UNORM;
	srcLocation.PlacedFootprint.Footprint.Width = width;
	srcLocation.PlacedFootprint.Footprint.Height = height;
	srcLocation.PlacedFootprint.Footprint.Depth = 1;
	srcLocation.PlacedFootprint.Footprint.RowPitch = uploadpitch;
	D3D12_TEXTURE_COPY_LOCATION dstLocation{};
	dstLocation.pResource = GetGPUResource();
	dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLocation.SubresourceIndex = 0;
	ctx.m_commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation,
										 nullptr);

	ctx.m_ri->TransitionResource(ctx.m_commandList, GetGPUResource(),
								 D3D12_RESOURCE_STATE_COPY_DEST,
								 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

auto D3DRITextureResource::ReleaseGPUResource() -> void
{
	m_gpuTextureResource.Reset();
	m_gpuUploadBufferResource.Reset();
}

auto D3DRITextureResource::GetGPUResource() -> ID3D12Resource*
{
	return m_gpuTextureResource.Get();
}

auto D3DRIIndexBufferResource::CreateGPUResource(
	D3DRIBatchResourceCreationContext& ctx) -> void
{
	D3DRIASSERT(!m_gpuIndexBufferResource);
	D3DRIASSERT(m_rawData);
	D3DRIASSERT(m_rawDataSizeInBytes > 0);

	// Create index buffer resource:
	// will be in D3D12_RESOURCE_STATE_COPY_DEST mode
	D3D12_RESOURCE_DESC indexBufferDesc{};
	indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	indexBufferDesc.Alignment = 0;
	indexBufferDesc.Width = m_rawDataSizeInBytes;
	indexBufferDesc.Height = 1;
	indexBufferDesc.DepthOrArraySize = 1;
	indexBufferDesc.MipLevels = 1;
	indexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	indexBufferDesc.SampleDesc.Count = 1;
	indexBufferDesc.SampleDesc.Quality = 0;
	indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	indexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	m_gpuIndexBufferResource =
		ctx.m_ri->CreateBufferResource(indexBufferDesc, D3D12_HEAP_TYPE_DEFAULT,
									   D3D12_RESOURCE_STATE_COPY_DEST);
	D3DRIASSERT(m_gpuIndexBufferResource);
	m_gpuIndexBufferResource->SetName(
		L"D3DRIIndexBufferResource::m_gpuIndexBufferResource");

	// Create upload buffer resource:
	// will be in D3D12_RESOURCE_STATE_GENERIC_READ mode
	D3D12_RESOURCE_DESC uploadBufferDesc{indexBufferDesc};

	m_gpuUploadBufferResource =
		ctx.m_ri->CreateBufferResource(uploadBufferDesc, D3D12_HEAP_TYPE_UPLOAD,
									   D3D12_RESOURCE_STATE_GENERIC_READ);
	D3DRIASSERT(m_gpuUploadBufferResource);
	m_gpuUploadBufferResource->SetName(
		L"D3DRIIndexBufferResource::m_gpuUploadBufferResource");
	// Write to the upload buffer:
	void* mapped{};
	D3D12_RANGE range{0, 0};
	auto hr = m_gpuUploadBufferResource->Map(0, &range, &mapped);
	if (D3DRI::IsError(hr, "Map"))
	{
		ctx.m_ri->FatalError("Map failed");
	}
	atd::memcpy(mapped, m_rawData, m_rawDataSizeInBytes);
	m_gpuUploadBufferResource->Unmap(0, &range);

	// Copy from the upload buffer to the index buffer:
	ctx.m_commandList->CopyBufferRegion(m_gpuIndexBufferResource.Get(), 0,
										m_gpuUploadBufferResource.Get(), 0,
										m_rawDataSizeInBytes);

	ctx.m_ri->TransitionResource(
		ctx.m_commandList, m_gpuIndexBufferResource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	// Create index buffer view:
	m_view.BufferLocation = m_gpuIndexBufferResource->GetGPUVirtualAddress();
	m_view.Format = m_format;
	m_view.SizeInBytes = static_cast<UINT>(m_rawDataSizeInBytes);
}

auto D3DRIIndexBufferResource::ReleaseGPUResource() -> void
{
	m_gpuIndexBufferResource.Reset();
	m_gpuUploadBufferResource.Reset();
	m_view = {};
}

auto D3DRIIndexBufferResource::GetGPUResource() -> ID3D12Resource*
{
	return m_gpuIndexBufferResource;
}

auto D3DRIIndexBufferResource::GetGPUIndexBufferView() const
	-> const D3D12_INDEX_BUFFER_VIEW&
{
	return m_view;
}

auto D3DRIVertexBufferResource::CreateGPUResource(
	D3DRIBatchResourceCreationContext& ctx) -> void
{
	D3DRIASSERT(!m_gpuVertexBufferResource);
	D3DRIASSERT(m_rawData);
	D3DRIASSERT(m_rawDataSizeInBytes > 0);

	// Create vertex buffer resource:
	D3D12_RESOURCE_DESC vertexBufferDesc{};
	vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferDesc.Alignment = 0;
	vertexBufferDesc.Width = m_rawDataSizeInBytes;
	vertexBufferDesc.Height = 1;
	vertexBufferDesc.DepthOrArraySize = 1;
	vertexBufferDesc.MipLevels = 1;
	vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexBufferDesc.SampleDesc.Count = 1;
	vertexBufferDesc.SampleDesc.Quality = 0;
	vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	m_gpuVertexBufferResource =
		ctx.m_ri->CreateBufferResource(vertexBufferDesc, D3D12_HEAP_TYPE_UPLOAD,
									   D3D12_RESOURCE_STATE_GENERIC_READ);

	// Write to the vertex buffer:
	void* mapped{};
	D3D12_RANGE range{0, 0};
	auto hr = m_gpuVertexBufferResource->Map(0, &range, &mapped);
	if (D3DRI::IsError(hr, "Map"))
	{
		ctx.m_ri->FatalError("Map failed");
	}
	atd::memcpy(mapped, m_rawData, m_rawDataSizeInBytes);
	m_gpuVertexBufferResource->Unmap(0, &range);

	// Create vertex buffer view:
	m_view.BufferLocation = m_gpuVertexBufferResource->GetGPUVirtualAddress();
	m_view.StrideInBytes = static_cast<UINT>(m_rawDataStrideInBytes);
	m_view.SizeInBytes = static_cast<UINT>(m_rawDataSizeInBytes);
}

auto D3DRIVertexBufferResource::ReleaseGPUResource() -> void
{
	m_gpuVertexBufferResource.Reset();

	m_view = {};
}

auto D3DRIVertexBufferResource::GetGPUResource() -> ID3D12Resource*
{
	return m_gpuVertexBufferResource.Get();
}

auto D3DRIVertexBufferResource::GetGPUVertexBufferView() const
	-> const D3D12_VERTEX_BUFFER_VIEW&
{
	return m_view;
}
