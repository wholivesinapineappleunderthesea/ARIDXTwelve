#pragma once

#include "../../Windows/COMPtr.h"

#include "../Common.h"
#define D3DRIASSERT(x) ASSERTCHECK(x)

#include "../VertexTypes.h"

#include <d3d12.h>
#include <dxgi1_3.h>

#define D3DRIDEBUG

#ifdef D3DRIDEBUG
#include <dxgidebug.h>
#endif

struct D3DRIResource;
struct D3DRITextureResource;
struct D3DRIVertexBufferResource;
struct D3DRIIndexBufferResource;
struct D3DRIResourceManager;

struct D3DRI
{
	D3DRI();
	~D3DRI();

	// The thread that calls CreateWindowAndResources will become the render
	// thread and is the only thread allowed to use DX resources.
	DWORD m_rendererThreadID{};
	auto IsRendererThread() const -> bool;

	// Window resources
	auto CreateWindowAndContext(const char* name = "D3D12 Window",
								int width = -1, int height = -1) -> bool;
	auto CheckMessageQueue() -> bool;
	HWND m_windowHandle{};
	atd::uint32_t m_windowWidth{};
	atd::uint32_t m_windowHeight{};
	float m_windowAspectRatio{};
	bool m_windowIsMinimized{};
	auto UpdateWindowSize(unsigned int w, unsigned int h) -> void;
	auto WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		-> LRESULT;

	// Error handling
	// Use to check if HRESULT is an error, logs any errors but does no action.
	static auto IsError(HRESULT hr, const char* text) -> bool;
#ifdef D3DRIDEBUG
#define D3DRIISERROR(hr, text) D3DRI::IsError(hr, text)
#else
#define D3DRIISERROR(hr, text) FAILED(hr)
#endif
	auto FatalError(const char* message) -> void;

#ifdef D3DRIDEBUG
	// Debug
	COMPtr<ID3D12Debug> m_debugD3D;
	COMPtr<IDXGIDebug1> m_debugDXGI{};
	auto CreateDebugInterfaces() -> void;
#endif

	static inline constexpr auto kNumSwapChainFrames = 2;

	// DXGI Factory
	COMPtr<IDXGIFactory3> m_dxgiFactory{};
	auto CreateFactory() -> void;
	// DXGI Adapter
	COMPtr<IDXGIAdapter1> m_dxgiAdapter{};
	D3D_FEATURE_LEVEL m_adapterFeatureLevel{};
	auto FindUsableAdapter(bool software) -> void;

	// D3D12 Device
	COMPtr<ID3D12Device> m_d3dDevice{};
	auto CreateDevice() -> void;

	// D3D12 Command Queue
	COMPtr<ID3D12CommandQueue> m_d3dCommandQueue{};
	auto CreateCommandQueue() -> void;

	// D3D12 Fence
	COMPtr<ID3D12Fence> m_d3dFence{};
	auto CreateFence() -> void;
	HANDLE m_fenceEvent{};
	auto SignalD3DFence() -> void;
	auto WaitForD3DFence() -> void;
	atd::uint64_t m_fenceValue{};
	atd::uint64_t m_fenceLastCompletedValue{};

	// D3D12 Command Allocators
	COMPtr<ID3D12CommandAllocator>
		m_d3dCommandAllocators[kNumSwapChainFrames]{};
	auto CreateCommandAllocators() -> void;

	// D3D12 Command List
	COMPtr<ID3D12GraphicsCommandList> m_d3dCommandList{};
	auto CreateCommandList() -> void;

	// D3D12 Root Signature
	COMPtr<ID3D12RootSignature> m_d3dRootSignature{};
	auto CreateRootSignature() -> void;
	struct Camera32BitConstants
	{
		float m_projectionMatrix[4][4]{};
		float m_viewMatrix[4][4]{};
		float m_viewportWidth{};
		float m_viewportHeight{};
		float m_time{};
	};
	static_assert(sizeof(Camera32BitConstants) % 4 == 0,
				  "Camera32BitConstants size must be a multiple of 4");
	struct Model32BitConstants
	{
		float m_modelMatrix[4][4]{};
		float m_uvScale[2]{};
		float m_uvOffset[2]{};
	};
	static_assert(sizeof(Model32BitConstants) % 4 == 0,
				  "Model32BitConstants size must be a multiple of 4");

	// D3D12 Descriptor Heaps
	COMPtr<ID3D12DescriptorHeap> m_d3dRTVDescriptorHeap{};
	UINT m_d3dRTVDescriptorSize{};
	COMPtr<ID3D12DescriptorHeap> m_d3dDSVDescriptorHeap{};
	UINT m_d3dDSVDescriptorSize{};
	COMPtr<ID3D12DescriptorHeap> m_d3dSRVDescriptorHeap{};
	UINT m_d3dSRVDescriptorSize{};
	auto CreateDescriptorHeaps() -> void;

	// Shader Compilation
	atd::uint8_t* m_3DVertexShader{};
	atd::size_t m_3DVertexShaderSize{};
	atd::uint8_t* m_3DPixelShader{};
	atd::size_t m_3DPixelShaderSize{};
	auto CompileShaderResources() -> void;

	// D3D12 Pipeline States
	COMPtr<ID3D12PipelineState> m_d3d3DPipelineState{};
	auto CreatePipelineStates() -> void;

	// Swap Chain
	COMPtr<IDXGISwapChain2> m_dxgiSwapChain{};
	auto CreateSwapChain() -> void;

	// D3D12 Render Targets
	COMPtr<ID3D12Resource> m_d3dRenderTargets[kNumSwapChainFrames]{};
	auto CreateRenderTargets() -> void;

	// D3D12 Depth Buffer
	COMPtr<ID3D12Resource> m_d3dDepthBuffer{};
	auto CreateDepthBuffer() -> void;

	// Viewport
	D3D12_VIEWPORT m_viewport{};
	// Scissor Rect
	D3D12_RECT m_scissorRect{};

	// Current frame index:
	int m_frameIndex{};
	atd::uintmax_t m_frameNumber{};

	// Current time:
	float m_time{};

	// Rendering:
	auto ShouldRender() -> bool;
	auto RenderFrame() -> void;
	auto FillAndExecuteCommandList() -> void;
	auto DeviceLost() -> void;
	auto ReleaseSwapChainDependantResources() -> void;
	auto CreateSwapChainDependantResources() -> void;
	auto ReleaseDeviceAndResources() -> void;
	auto CreateDeviceAndResources() -> void;
	auto TransitionResource(ID3D12GraphicsCommandList* list,
							ID3D12Resource* resource,
							D3D12_RESOURCE_STATES before,
							D3D12_RESOURCE_STATES after) -> void;
	auto CreateBufferResource(
		const D3D12_RESOURCE_DESC& desc,
		D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ)
		-> COMPtr<ID3D12Resource>;

	// Updating:
	auto Update(float deltaTime) -> void;

	// Resources:
	D3DRIResourceManager* m_resourceManager{};
	auto GetResourceManager() -> D3DRIResourceManager*;
	// Called before RenderFrame to create any rendering resources that are
	// needed.
	auto ValidateAndCreateGPUResources() -> void;

	// Generic resources:
	D3DRIVertexBufferResource* m_genericCubeResource{};
	auto CreateGenericResources() -> void;
};

struct D3DRIRenderFrameContext
{
	ID3D12CommandQueue* m_queue{};
	ID3D12CommandAllocator* m_alloc{};
	ID3D12Resource* m_target{};
	ID3D12GraphicsCommandList* m_list{};

	D3DRI* m_ri{};

	using Camera32BitConstants = D3DRI::Camera32BitConstants;
using Model32BitConstants = D3DRI::Model32BitConstants;

	Camera32BitConstants m_cameraConstants{};
	Model32BitConstants m_modelConstants{};

	auto SubmitCameraConstants() -> void;
	auto SubmitModelConstants() -> void;

	inline auto SetCameraConstants(const Camera32BitConstants& constants)
		-> void
	{
		m_cameraConstants = constants;
	}
	inline auto SetModelConstants(const Model32BitConstants& constants)
		-> void
	{
		m_modelConstants = constants;
	}

	auto FillCommandList() -> void;
};

inline D3DRI* g_D3DRI{};