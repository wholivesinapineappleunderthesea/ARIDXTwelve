#pragma once

#include "D3DRIResource.h"

#include "../Common.h"

struct D3DRIResourceManager
{
	D3DRIResourceManager();
	~D3DRIResourceManager();

// May be called by any thread
	template <typename T, typename... Args>
	inline auto CreateResource(Args&&... args) -> T*
	{
		auto resource = new T(atd::forward<Args>(args)...);
		CreateResourceInternal(resource);
		return resource;
	}
	auto DestroyResource(D3DRIResource* resource) -> void;

	// To be called by D3DRI's render thread only!
	// Called if the device is lost
	auto ReleaseGPUResources() -> void;
	// Loops over m_resources and makes sure the gpu resource is created
	auto CreateGPUResources(D3DRIBatchResourceCreationContext& ctx) -> void;
	// Number of resources that need to be created
	auto GetNumResourcesNotCreated() const -> size_t;

  private:
	auto CreateResourceInternal(D3DRIResource* resource) -> void;
	Mutex m_mutex{};
	Array<D3DRIResource*> m_resources{};
	size_t m_numResourcesNotCreated{};
};