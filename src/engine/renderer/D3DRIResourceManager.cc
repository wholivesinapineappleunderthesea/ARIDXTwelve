#include "D3DRIResourceManager.h"

D3DRIResourceManager::D3DRIResourceManager()
{
}

D3DRIResourceManager::~D3DRIResourceManager()
{
	m_mutex.lock();
	for (auto p : m_resources)
	{
		delete p;
	}
	m_mutex.unlock();
}
auto D3DRIResourceManager::DestroyResource(D3DRIResource* resource) -> void
{
	m_mutex.lock();
	for (auto it = m_resources.begin(); it != m_resources.end(); it++)
	{
		if (*it == resource)
		{
			it = m_resources.Erase(it);
			break;
		}
	}
	delete resource;
	m_mutex.unlock();
}

auto D3DRIResourceManager::ReleaseGPUResources() -> void
{
	m_mutex.lock();
	for (auto p : m_resources)
	{
		if (p->GetGPUResource())
		{
			p->ReleaseGPUResource();
			m_numResourcesNotCreated++;
		}
	}
	m_mutex.unlock();
}

auto D3DRIResourceManager::CreateGPUResources(
	D3DRIBatchResourceCreationContext& ctx) -> void
{
	m_mutex.lock();

	const auto numToCreate = GetNumResourcesNotCreated();

	for (auto p : m_resources)
	{
		if (!p->GetGPUResource())
		{
			p->CreateGPUResource(ctx);
			ctx.m_numResourcesCreated++;
			m_numResourcesNotCreated--;
		}
	}

	D3DRIASSERT(numToCreate == ctx.m_numResourcesCreated);

	m_mutex.unlock();
}

auto D3DRIResourceManager::GetNumResourcesNotCreated() const -> size_t
{
	return m_numResourcesNotCreated;
}

auto D3DRIResourceManager::CreateResourceInternal(D3DRIResource* resource)
	-> void
{
	m_mutex.lock();
	m_resources.Insert(resource);
	m_numResourcesNotCreated++;
	m_mutex.unlock();
}
