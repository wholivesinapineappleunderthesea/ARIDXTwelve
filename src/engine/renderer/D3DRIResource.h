#pragma once

#include "D3DRI.h"

enum D3DRIResourceType
{
	Texture,
	VertexBuffer,
	IndexBuffer,
};

struct D3DRIBatchResourceCreationContext
{
	D3DRI* m_ri{};
	ID3D12Device* m_device{};
	ID3D12GraphicsCommandList* m_commandList{};
	ID3D12CommandAllocator* m_commandAllocator{};
	ID3D12CommandQueue* m_commandQueue{};

	size_t m_numResourcesCreated{};
};

struct D3DRIResource
{

	auto GetType() const -> D3DRIResourceType;

	// Will be called by D3DRIResourceManager.
	// The GPU resource should only be accessed on the render thread!
	virtual auto CreateGPUResource(D3DRIBatchResourceCreationContext& ctx)
		-> void = 0;
	virtual auto ReleaseGPUResource() -> void = 0;
	virtual auto GetGPUResource() -> ID3D12Resource* = 0;

	// Should be called by D3DRIResourceManager.
	virtual ~D3DRIResource();

  protected:
	D3DRIResource(D3DRIResourceType type, const atd::uint8_t* data,
				  atd::size_t sizeBytes, atd::size_t strideBytes = 0);
	D3DRIResourceType m_type{};

	atd::uint8_t* m_rawData{};
	atd::size_t m_rawDataSizeInBytes{};
	atd::size_t m_rawDataStrideInBytes{};
};

struct D3DRITextureResource : public D3DRIResource
{
	// Will be called by D3DRIResourceManager.
	// The GPU resource should only be accessed on the render thread!
	inline D3DRITextureResource(const atd::uint8_t* data, atd::size_t sizeBytes,
								unsigned int width, unsigned int height,
								unsigned int depth, unsigned int numMip,
								DXGI_FORMAT fmt)
		: D3DRIResource(D3DRIResourceType::Texture, data, sizeBytes),
		  m_width(width),
		  m_height(height),
		  m_depth(depth),
		  m_mipLevels(numMip),
		  m_format(fmt)
	{
	}
	auto CreateGPUResource(D3DRIBatchResourceCreationContext& ctx)
		-> void override;
	auto ReleaseGPUResource() -> void override;
	auto GetGPUResource() -> ID3D12Resource* override;

  private:
	COMPtr<ID3D12Resource> m_gpuTextureResource{};
	COMPtr<ID3D12Resource> m_gpuUploadBufferResource{};
	unsigned int m_width{};
	unsigned int m_height{};
	unsigned int m_depth{};
	unsigned int m_mipLevels{};
	DXGI_FORMAT m_format{};
};

struct D3DRIIndexBufferResource : public D3DRIResource
{
	// Will be called by D3DRIResourceManager.
	// The GPU resource should only be accessed on the render thread!
	inline D3DRIIndexBufferResource(const atd::uint8_t* data,
									atd::size_t sizeBytes,
									atd::size_t bytesPerElement,
									DXGI_FORMAT fmt)
		: D3DRIResource(D3DRIResourceType::IndexBuffer, data, sizeBytes,
						bytesPerElement),
		  m_format(fmt)
	{
	}
	auto CreateGPUResource(D3DRIBatchResourceCreationContext& ctx)
		-> void override;
	auto ReleaseGPUResource() -> void override;
	auto GetGPUResource() -> ID3D12Resource* override;

	auto GetGPUIndexBufferView() const -> const D3D12_INDEX_BUFFER_VIEW&;

	// Bytes per index element
	inline auto GetBytesPerElement() const -> atd::size_t
	{
		return m_rawDataStrideInBytes;
	}

  private:
	COMPtr<ID3D12Resource> m_gpuIndexBufferResource{};
	COMPtr<ID3D12Resource> m_gpuUploadBufferResource{};
	D3D12_INDEX_BUFFER_VIEW m_view{};
	DXGI_FORMAT m_format{};
};

struct D3DRIVertexBufferResource : public D3DRIResource
{
	// Will be called by D3DRIResourceManager.
	// The GPU resource should only be accessed on the render thread!
	inline D3DRIVertexBufferResource(const atd::uint8_t* data,
									 atd::size_t sizeBytes,
									 atd::size_t strideBytes)
		: D3DRIResource(D3DRIResourceType::VertexBuffer, data, sizeBytes,
						strideBytes)
	{
	}
	auto CreateGPUResource(D3DRIBatchResourceCreationContext& ctx)
		-> void override;
	auto ReleaseGPUResource() -> void override;
	auto GetGPUResource() -> ID3D12Resource* override;
	auto GetGPUVertexBufferView() const -> const D3D12_VERTEX_BUFFER_VIEW&;

	// Bytes per vertex
	inline auto GetBytesPerVertex() const -> atd::size_t
	{
		return m_rawDataStrideInBytes;
	}

  private:
	COMPtr<ID3D12Resource> m_gpuVertexBufferResource{};
	D3D12_VERTEX_BUFFER_VIEW m_view{};
};