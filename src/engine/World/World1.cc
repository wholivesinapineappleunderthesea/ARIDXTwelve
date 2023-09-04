#include "World1.h"
// World1Mesh
#include "../../embed/resources.h"
#include "../Entity/Component/BoundingBoxComponent.h"
#include "../Entity/Component/CameraComponent.h"
#include "../Entity/Component/StaticMeshD3DRIRendererComponent.h"
#include "../Entity/Entity.h"
// Vertex3D*
#include "../VertexTypes.h"
// For D3DRIResourceManager::CreateResource
#include "../renderer/D3DRI.h"
#include "../renderer/D3DRIResource.h"
#include "../renderer/D3DRIResourceManager.h"

World1::World1() : World()
{
	SetWorldName("World1");
	CreatePlayerEntity();
	CreateStaticGeometryEntity();
}

World1::~World1()
{
	delete m_defaultPlayerEntity;
	delete m_worldMeshEntity;
}

auto World1::Update(float deltaTime) -> void
{
	Super::Update(deltaTime);
}

auto World1::GetRenderingCameraComponent() -> CameraComponent*
{
	return Super::GetRenderingCameraComponent();
}

auto World1::CreateStaticGeometryEntity() -> void
{
	const auto& meshBytes = World1Mesh;
	// const auto meshsize = sizeof(World1Mesh);

	const auto bytesPerIndex = *reinterpret_cast<const uint8_t*>(meshBytes);
	const auto numVertices = *reinterpret_cast<const uint32_t*>(meshBytes + 1);
	const auto numIndices = *reinterpret_cast<const uint32_t*>(meshBytes + 5);
	ASSERTCHECK(bytesPerIndex);
	ASSERTCHECK(numVertices);
	ASSERTCHECK(numIndices);

	DXGI_FORMAT indexFmt{DXGI_FORMAT_UNKNOWN};
	switch (bytesPerIndex)
	{
	case 1:
		indexFmt = DXGI_FORMAT_R8_UINT;
		break;
	case 2:
		indexFmt = DXGI_FORMAT_R16_UINT;
		break;
	case 4:
		indexFmt = DXGI_FORMAT_R32_UINT;
		break;
	default:
		break;
	};
	ASSERTCHECK(indexFmt != DXGI_FORMAT_UNKNOWN);

	const auto verticesStart =
		reinterpret_cast<const Vertex3DNoColour*>(meshBytes + 9);
	const auto indicesStart =
		reinterpret_cast<const uint8_t*>(verticesStart + numVertices);
	const auto newVerts = new Vertex3D[numVertices];
	for (atd::uint32_t i = 0; i < numVertices; ++i)
	{
		Vertex3D v{};
		v.x = verticesStart[i].x;
		v.y = verticesStart[i].y;
		v.z = verticesStart[i].z;
		v.nx = verticesStart[i].nx;
		v.ny = verticesStart[i].ny;
		v.nz = verticesStart[i].nz;
		v.u = verticesStart[i].u;
		v.v = verticesStart[i].v;
		newVerts[i] = v;
		Math::Vec4 col{v.u, v.v, 0.f, 1.f};
		// v.col = ABGR
		v.col = (static_cast<atd::uint32_t>(col.w * 255.f) << 24) |
				(static_cast<atd::uint32_t>(col.z * 255.f) << 16) |
				(static_cast<atd::uint32_t>(col.y * 255.f) << 8) |
				(static_cast<atd::uint32_t>(col.x * 255.f));
		newVerts[i] = v;
	}

	const auto ri = g_D3DRI;
	const auto rm = ri->GetResourceManager();
	const auto vbResource = rm->CreateResource<D3DRIVertexBufferResource>(
		reinterpret_cast<const uint8_t*>(newVerts),
		numVertices * sizeof(Vertex3D), sizeof(Vertex3D));
	const auto ibResource = rm->CreateResource<D3DRIIndexBufferResource>(
		indicesStart, numIndices * bytesPerIndex, bytesPerIndex, indexFmt);

	m_worldMeshEntity = new Entity{};
	m_worldMeshEntity->SetPosition(Math::Vec3{0.0f, 0.0f, 0.0f}, false);
	m_worldMeshEntity->SetScale(Math::Vec3{1.0f, 1.0f, 1.0f}, false);
	// rotate 90 degrees around x axis
	m_worldMeshEntity->SetRotation(
		Math::Quat{0.70710678118f, 0.f, 0.f, 0.70710678118f}, false);
	m_worldMeshEntity->SetName("WorldMeshEntity");
	m_worldMeshEntity->AttachToWorld(this);
	const auto renderer =
		m_worldMeshEntity->AddComponent<StaticMeshD3DRIRendererComponent>();
	renderer->SetVertexBuffer(vbResource, true);
	renderer->SetIndexBuffer(ibResource, true);

	m_worldMeshEntity->SetRenderable(true);

	delete newVerts;
}

auto World1::CreatePlayerEntity() -> void
{
	m_defaultPlayerEntity = new Entity{};
	m_defaultPlayerEntity->SetPosition(Math::Vec3{-2.0f, 5.0f, -2.0f}, false);
	m_defaultPlayerEntity->SetScale(Math::Vec3{1.0f, 1.0f, 1.0f}, false);
	m_defaultPlayerEntity->SetRotation(Math::Quat{0.0f, 0.0f, 0.0f, 1.0f},
									   false);
	m_defaultPlayerEntity->SetName("World1DefaultPlayerEntity");
	m_defaultPlayerEntity->AttachToWorld(this);
	m_defaultPlayerEntity->SetCanUpdate(true);
	const auto box =
		m_defaultPlayerEntity->AddComponent<BoundingBoxComponent>();
	box->SetMinAABBOffset(Math::Vec3{-0.5f, -0.5f, -0.5f});
	box->SetMaxAABBOffset(Math::Vec3{0.5f, 0.5f, 0.5f});
	m_defaultPlayerEntity->SetPhysicsEnabled(1);

	const auto cam = m_defaultPlayerEntity->AddComponent<CameraComponent>();
	cam->SetFOV(160.f);
	cam->SetCanMouseLook(1);
	//cam->SetInFreeCamMode(1);
	cam->SetCameraEntityOffset(Math::Vec3{0.f, 0.5f, 0.f});
	cam->SetShouldUpdateFromEntityPosition(1);
	SetCurrentLocalPlayerEntity(m_defaultPlayerEntity);
}
