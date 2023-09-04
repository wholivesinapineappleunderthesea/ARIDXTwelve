#include "D3DRI.h"
#include "../../embed/resources.h"
#include "../GlobalWindowData.h"
#include "D3DRIResourceManager.h"

#include <d3dcompiler.h>

D3DRI::D3DRI()
{
	CompileShaderResources();
	m_resourceManager = new D3DRIResourceManager{};
	CreateGenericResources();
}

D3DRI::~D3DRI()
{
	const auto fence = m_d3dFence;
	if (fence)
	{
		SignalD3DFence();
		WaitForD3DFence();
		CloseHandle(m_fenceEvent);
	}

	FreeMemory(m_3DVertexShader);
	FreeMemory(m_3DPixelShader);

	// The destructor of the resource manager will release all resources so we
	// can ignore them in this destructor
	delete m_resourceManager;
}

// Just a central point for notifying of errors, not used for handling them
auto D3DRI::IsError(HRESULT hr, const char* text) -> bool
{
	(void)text;
	if (!SUCCEEDED(hr))
	{
		LogWinError(LogLevel::Error, hr, text);
		return true;
	}
	return false;
}

auto D3DRI::FatalError(const char* message) -> void
{
	LogMessage(LogLevel::Fatal, message);
	D3DRIASSERT(0);
}

auto D3DRIWindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
	-> LRESULT
{
	if (msg == WM_CREATE)
	{
		const auto cs = reinterpret_cast<CREATESTRUCTA*>(lparam);
		D3DRIASSERT(cs);
		D3DRIASSERT(cs->lpCreateParams);
		SetWindowLongPtrA(wnd, 0,
						  reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
	}
	const auto d3dri = reinterpret_cast<D3DRI*>(GetWindowLongPtrA(wnd, 0));
	if (d3dri)
		return d3dri->WindowProc(wnd, msg, wparam, lparam);
	return DefWindowProcA(wnd, msg, wparam, lparam);
}

auto D3DRI::CreateWindowAndContext(const char* name, int w, int h) -> bool
{
	m_rendererThreadID = GetCurrentThreadId();
	if (w == -1)
		w = CW_USEDEFAULT;
	if (h == -1)
		h = CW_USEDEFAULT;

	WNDCLASSEXA wcex{};
	wcex.cbSize = sizeof wcex;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = D3DRIWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(void*);
	wcex.hInstance = GetModuleHandleA(nullptr);
	wcex.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
	wcex.hCursor = LoadCursorA(nullptr, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(5);
	wcex.lpszClassName = "D3DAri";

	wcex.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);

	if (!RegisterClassExA(&wcex))
		return false;

	m_windowHandle = CreateWindowExA(
		0, wcex.lpszClassName, name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
		CW_USEDEFAULT, w, h, nullptr, nullptr, wcex.hInstance, this);
	if (!m_windowHandle)
		return false;

	ShowWindow(m_windowHandle, 1);
	UpdateWindow(m_windowHandle);

	RAWINPUTDEVICE rawinput;
	rawinput.usUsagePage = ((USHORT)0x01); // HID_USAGE_PAGE_GENERIC
	rawinput.usUsage = ((USHORT)0x02);	   // HID_USAGE_GENERIC_MOUSE
	rawinput.dwFlags = RIDEV_INPUTSINK;
	rawinput.hwndTarget = m_windowHandle;
	D3DRIASSERT(RegisterRawInputDevices(&rawinput, 1, sizeof rawinput));

	// Get width and height
	RECT rect{};
	GetClientRect(m_windowHandle, &rect);
	UpdateWindowSize(rect.right - rect.left, rect.bottom - rect.top);
#ifdef D3DRIDEBUG
	CreateDebugInterfaces();
#endif

	CreateFactory();

	FindUsableAdapter(false);
	if (!m_dxgiAdapter)
		FindUsableAdapter(true);
	D3DRIASSERT(m_dxgiAdapter);

	CreateDeviceAndResources();
	return true;
}

auto D3DRI::CheckMessageQueue() -> bool
{

	bool shouldQuit{};
	MSG msg{};
	while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			shouldQuit = true;
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return shouldQuit;
}

auto D3DRI::IsRendererThread() const -> bool
{
	return m_rendererThreadID == GetCurrentThreadId();
}

auto D3DRI::UpdateWindowSize(unsigned int w, unsigned int h) -> void
{

	if (w != m_windowWidth || h != m_windowHeight)
	{
		m_windowWidth = w;
		m_windowHeight = h;
		m_windowAspectRatio = static_cast<float>(w) / static_cast<float>(h);
		g_windowData->m_width = w;
		g_windowData->m_height = h;
		g_windowData->m_aspectRatio = m_windowAspectRatio;

		g_windowData->m_callbacks.Lock();
		for (auto& callback : g_windowData->m_callbacks.m_onWindowResize)
			callback(g_windowData->m_width, m_windowHeight);
		g_windowData->m_callbacks.Unlock();

		m_scissorRect.left = 0;
		m_scissorRect.top = 0;
		m_scissorRect.right = static_cast<LONG>(w);
		m_scissorRect.bottom = static_cast<LONG>(h);

		m_viewport.Height = static_cast<float>(h);
		m_viewport.Width = static_cast<float>(w);
		m_viewport.MaxDepth = 1.0f;
		m_viewport.MinDepth = 0.0f;
		m_viewport.TopLeftX = 0.0f;
		m_viewport.TopLeftY = 0.0f;

		if (m_dxgiSwapChain)
		{
			if (m_d3dFence)
			{
				SignalD3DFence();
				WaitForD3DFence();
			}
			ReleaseSwapChainDependantResources();

			DXGI_SWAP_CHAIN_DESC1 desc{};
			auto hr = m_dxgiSwapChain->GetDesc1(&desc);
			if (IsError(hr, "Failed to get swap chain description"))
				FatalError("Failed to get swap chain description");

			hr = m_dxgiSwapChain->ResizeBuffers(0, w, h, desc.Format,
												desc.Flags);
			if (hr == DXGI_ERROR_DEVICE_REMOVED ||
				hr == DXGI_ERROR_DEVICE_RESET)
			{
				LogWinError(LogLevel::Warning, hr, "ResizeBuffers");
				if (m_d3dDevice)
				{
					auto removedReason = m_d3dDevice->GetDeviceRemovedReason();
					LogWinError(LogLevel::Warning, removedReason,
								"GetDeviceRemovedReason");
				}
				DeviceLost();
			}
			else if (D3DRIISERROR(hr, "ResizeBuffers"))
			{
				FatalError("Unable to resize swap chain!");
			}
			CreateSwapChainDependantResources();
		}
	}
}

auto D3DRI::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	-> LRESULT
{
	switch (message)
	{
	case WM_INPUT: {
		static BYTE lpb[sizeof(RAWINPUT)];

		UINT dwSize = sizeof lpb;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb,
						&dwSize, sizeof(RAWINPUTHEADER));

		const auto raw = reinterpret_cast<RAWINPUT*>(lpb);
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			const auto xrel = raw->data.mouse.lLastX;
			const auto yrel = raw->data.mouse.lLastY;
			g_windowData->m_callbacks.Lock();
			for (auto& c : g_windowData->m_callbacks.m_onMouseMove)
				c(xrel, yrel);
			g_windowData->m_callbacks.Unlock();
		}
		return 0;
	}
	case WM_KEYDOWN: {
		if (wParam < 256)
		{
			g_windowData->m_keyStates[wParam] = true;
		}
		break;
	}
	case WM_KEYUP: {
		if (wParam < 256)
		{
			g_windowData->m_keyStates[wParam] = false;
		}
		break;
	}
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
		{
			m_windowIsMinimized = true;
			g_windowData->m_isMinimized = true;
		}
		if (wParam == SIZE_MAXIMIZED)
		{
			m_windowIsMinimized = false;
			g_windowData->m_isMinimized = false;
		}
		if (!m_windowIsMinimized)
			UpdateWindowSize(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	};
	return DefWindowProcA(hWnd, message, wParam, lParam);
}
#ifdef D3DRIDEBUG
auto D3DRI::CreateDebugInterfaces() -> void
{

	auto hr = D3D12GetDebugInterface(m_debugD3D.static_uuid, m_debugD3D.Out());
	if (!D3DRIISERROR(hr, "D3D12GetDebugInterface"))
	{
		D3DRIASSERT(m_debugD3D);
		m_debugD3D->EnableDebugLayer();
	}
	else
	{
		LogMessage(LogLevel::Warning, "Failed to create D3D12 debug interface");
	}

	hr = DXGIGetDebugInterface1(0, m_debugDXGI.static_uuid, m_debugDXGI.Out());
	if (!D3DRIISERROR(hr, "DXGIGetDebugInterface1"))
	{
		D3DRIASSERT(m_debugDXGI);
		m_debugDXGI->EnableLeakTrackingForThread();
	}
	else
	{
		LogMessage(LogLevel::Warning, "Failed to create DXGI debug interface");
	}
}

#endif

auto D3DRI::CreateFactory() -> void
{

	UINT flags{};
#ifdef D3DRIDEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	auto hr = CreateDXGIFactory2(flags, m_dxgiFactory.static_uuid,
								 m_dxgiFactory.Out());
	if (D3DRIISERROR(hr, "CreateDXGIFactory2"))
	{
		FatalError("Unable to create DXGI factory");
	}
	D3DRIASSERT(m_dxgiFactory);
}

auto D3DRI::FindUsableAdapter(bool allowSoftware) -> void
{

	D3DRIASSERT(m_dxgiFactory);
	COMPtr<IDXGIAdapter1> adapter{};
	for (UINT idx{};
		 !D3DRIISERROR(m_dxgiFactory->EnumAdapters1(idx, adapter.InOut()),
					   "dxgiFactory->EnumAdapters1");
		 ++idx)
	{
		if (!adapter)
			continue;
		DXGI_ADAPTER_DESC1 adapterDesc{};
		if (D3DRIISERROR(adapter->GetDesc1(&adapterDesc), "adapter->GetDesc1"))
			continue;
		if (!allowSoftware && adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		const auto test = [&](D3D_FEATURE_LEVEL level) {
			auto hr = ::D3D12CreateDevice(adapter.Get(), level,
										  __uuidof(ID3D12Device), 0);
			if (SUCCEEDED(hr))
			{
				m_adapterFeatureLevel = level;
				return true;
			}
			return false;
		};
		bool canUse = test(D3D_FEATURE_LEVEL_12_2);
		if (!canUse)
			canUse = test(D3D_FEATURE_LEVEL_12_1);
		if (!canUse)
			canUse = test(D3D_FEATURE_LEVEL_12_0);
		if (canUse)
		{
			m_dxgiAdapter = atd::move(adapter);
			return;
		}
	}
}

auto D3DRI::CreateDevice() -> void
{

	D3DRIASSERT(m_dxgiAdapter);

	const auto adapter = m_dxgiAdapter;
	auto hr = D3D12CreateDevice(adapter, m_adapterFeatureLevel,
								m_d3dDevice.static_uuid, m_d3dDevice.Out());
	if (D3DRIISERROR(hr, "D3D12CreateDevice"))
	{
		FatalError("Unable to create D3D12 device!");
	}
	D3DRIASSERT(m_d3dDevice);

	m_d3dDevice->SetName(L"D3DRI::m_d3dDevice");
}

auto D3DRI::CreateCommandQueue() -> void
{

	D3DRIASSERT(m_d3dDevice);
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	auto hr = m_d3dDevice->CreateCommandQueue(
		&desc, m_d3dCommandQueue.static_uuid, m_d3dCommandQueue.Out());
	if (D3DRIISERROR(hr, "device->CreateCommandQueue"))
	{
		FatalError("Unable to create command queue!");
	}
	D3DRIASSERT(m_d3dCommandQueue);

	m_d3dCommandQueue->SetName(L"D3DRI::m_d3dCommandQueue");
}

auto D3DRI::CreateCommandAllocators() -> void
{

	D3DRIASSERT(m_d3dDevice);

	for (auto i = 0; i < kNumSwapChainFrames; i++)
	{
		auto hr = m_d3dDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_d3dCommandAllocators[i].static_uuid,
			m_d3dCommandAllocators[i].Out());
		if (D3DRIISERROR(hr, "device->CreateCommandAllocator"))
		{
			FatalError("Unable to create command allocator!");
		}
		D3DRIASSERT(m_d3dCommandAllocators[i]);

		m_d3dCommandAllocators[i]->SetName(L"D3DRI::m_d3dCommandAllocators");
	}
}

auto D3DRI::CreateCommandList() -> void
{

	D3DRIASSERT(m_d3dDevice);
	D3DRIASSERT(m_d3dCommandAllocators[0]);

	auto hr = m_d3dDevice->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_d3dCommandAllocators[0], nullptr,
		m_d3dCommandList.static_uuid, m_d3dCommandList.Out());
	if (D3DRIISERROR(hr, "device->CreateCommandList"))
	{
		FatalError("Unable to create command list!");
	}
	D3DRIASSERT(m_d3dCommandList);

	m_d3dCommandList->SetName(L"D3DRI::m_d3dCommandList");

	hr = m_d3dCommandList->Close();
	if (D3DRIISERROR(hr, "commandList->Close"))
	{
		FatalError("Unable to close command list!");
	}
}

auto D3DRI::CreateFence() -> void
{

	D3DRIASSERT(m_d3dDevice);

	auto hr = m_d3dDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, m_d3dFence.static_uuid, m_d3dFence.Out());
	if (D3DRIISERROR(hr, "device->CreateFence"))
	{
		FatalError("Unable to create fence!");
	}
	D3DRIASSERT(m_d3dFence);

	m_d3dFence->SetName(L"D3DRI::m_d3dFence");
	if (!m_fenceEvent)
		m_fenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);

	m_fenceValue = 0;
}

auto D3DRI::SignalD3DFence() -> void
{

	D3DRIASSERT(m_d3dCommandQueue);

	auto hr = m_d3dCommandQueue->Signal(m_d3dFence, m_fenceValue);
	if (D3DRIISERROR(hr, "commandQueue->Signal"))
	{
		FatalError("Unable to signal fence!");
	}
}

auto D3DRI::WaitForD3DFence() -> void
{

	D3DRIASSERT(m_d3dFence);
	D3DRIASSERT(m_fenceEvent);

	if (m_d3dFence->GetCompletedValue() < m_fenceValue)
	{
		auto hr = m_d3dFence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
		if (D3DRIISERROR(hr, "fence->SetEventOnCompletion"))
		{
			FatalError("Unable to set fence event!");
		}

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	m_fenceValue++;
}

auto D3DRI::CreateRootSignature() -> void
{

	D3DRIASSERT(m_d3dDevice);

	D3D12_DESCRIPTOR_RANGE descRange{};
	descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange.NumDescriptors = 1;
	descRange.BaseShaderRegister = 0;
	descRange.RegisterSpace = 0;
	descRange.OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER param[3]{};
	param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	param[0].Constants.ShaderRegister = 0;
	param[0].Constants.RegisterSpace = 0;
	param[0].Constants.Num32BitValues = sizeof(Camera32BitConstants) / 4;
	param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	param[1].Constants.ShaderRegister = 1;
	param[1].Constants.RegisterSpace = 0;
	param[1].Constants.Num32BitValues = sizeof(Model32BitConstants) / 4;
	param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	param[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param[2].DescriptorTable.NumDescriptorRanges = 1;
	param[2].DescriptorTable.pDescriptorRanges = &descRange;
	param[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC staticSampler{};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSampler.MipLODBias = 0.f;
	staticSampler.MaxAnisotropy = 0;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSampler.MinLOD = 0.f;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0;
	staticSampler.RegisterSpace = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = _countof(param);
	// desc.NumParameters = 0;
	desc.pParameters = param;
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = &staticSampler;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				 D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				 D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				 D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	COMPtr<ID3DBlob> signature;
	COMPtr<ID3DBlob> error;
	auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
										  signature.Out(), error.Out());
	if (D3DRIISERROR(hr, "D3D12SerializeRootSignature"))
	{
		FatalError("Unable to serialize root signature!");
	}
	D3DRIASSERT(signature);
	hr = m_d3dDevice->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(),
		m_d3dRootSignature.static_uuid, m_d3dRootSignature.Out());
	if (D3DRIISERROR(hr, "device->CreateRootSignature"))
	{
		FatalError("Unable to create root signature!");
	}
	D3DRIASSERT(m_d3dRootSignature);
	m_d3dRootSignature->SetName(L"m_d3dRootSignature");
}

auto D3DRI::CreateDescriptorHeaps() -> void
{

	D3DRIASSERT(m_d3dDevice);
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = kNumSwapChainFrames;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		auto hr = m_d3dDevice->CreateDescriptorHeap(
			&desc, m_d3dRTVDescriptorHeap.static_uuid,
			m_d3dRTVDescriptorHeap.Out());
		if (D3DRIISERROR(hr, "device->CreateDescriptorHeap"))
		{
			FatalError("Unable to create RTV descriptor heap!");
		}
		D3DRIASSERT(m_d3dRTVDescriptorHeap);

		m_d3dRTVDescriptorHeap->SetName(L"D3DRI::m_d3dRTVDescriptorHeap");

		m_d3dRTVDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3DRIASSERT(m_d3dRTVDescriptorSize);
	}
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		auto hr = m_d3dDevice->CreateDescriptorHeap(
			&desc, m_d3dDSVDescriptorHeap.static_uuid,
			m_d3dDSVDescriptorHeap.Out());
		if (D3DRIISERROR(hr, "device->CreateDescriptorHeap"))
		{
			FatalError("Unable to create DSV descriptor heap!");
		}
		D3DRIASSERT(m_d3dDSVDescriptorHeap);

		m_d3dDSVDescriptorHeap->SetName(L"D3DRI::m_d3dDSVDescriptorHeap");

		m_d3dDSVDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		D3DRIASSERT(m_d3dDSVDescriptorSize);
	}
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = 0;

		auto hr = m_d3dDevice->CreateDescriptorHeap(
			&desc, m_d3dSRVDescriptorHeap.static_uuid,
			m_d3dSRVDescriptorHeap.Out());
		if (D3DRIISERROR(hr, "device->CreateDescriptorHeap"))
		{
			FatalError("Unable to create SRV descriptor heap!");
		}
		D3DRIASSERT(m_d3dSRVDescriptorHeap);

		m_d3dSRVDescriptorHeap->SetName(L"D3DRI::m_d3dSRVDescriptorHeap");

		m_d3dSRVDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3DRIASSERT(m_d3dSRVDescriptorSize);
	}
}

auto D3DRI::CompileShaderResources() -> void
{
	auto compileShader =
		[this](atd::uint8_t*& bytecode, atd::size_t& bytecodeSize,
			   const atd::uint8_t* source, atd::size_t sourceSize,
			   const char* target) -> void {
		COMPtr<ID3DBlob> bin{}, error{};
		auto hr = D3DCompile(source, sourceSize, nullptr, nullptr, nullptr,
							 "main", target, 0, 0, bin.Out(), error.Out());

		if (D3DRIISERROR(hr, "D3DCompile"))
		{
			if (error)
			{
				char* buf = reinterpret_cast<char*>(
					AllocateMemory(error->GetBufferSize() + 64));
				atd::strcpy(buf, "Unable to compile shader!\n");
				atd::strcat(
					buf,
					reinterpret_cast<const char*>(error->GetBufferPointer()),
					error->GetBufferSize());
				FatalError(buf);
				// FreeMemory(buf); // unreachable
			}
			else
			{
				FatalError("Unable to compile shader!");
			}
		}
		D3DRIASSERT(bin);
		bytecodeSize = bin->GetBufferSize();
		bytecode =
			reinterpret_cast<atd::uint8_t*>(AllocateMemory(bytecodeSize));
		atd::memcpy(bytecode, bin->GetBufferPointer(), bin->GetBufferSize());
	};
	compileShader(m_3DVertexShader, m_3DVertexShaderSize, Primary3DVS,
				  sizeof Primary3DVS, "vs_5_0");
	compileShader(m_3DPixelShader, m_3DPixelShaderSize, Primary3DPS,
				  sizeof Primary3DPS, "ps_5_0");
}

auto D3DRI::CreatePipelineStates() -> void
{

	D3DRIASSERT(m_d3dDevice);
	D3DRIASSERT(m_d3dRootSignature);
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = m_d3dRootSignature.Get();

	psoDesc.VS.pShaderBytecode = m_3DVertexShader;
	psoDesc.VS.BytecodeLength = m_3DVertexShaderSize;

	psoDesc.PS.pShaderBytecode = m_3DPixelShader;
	psoDesc.PS.BytecodeLength = m_3DPixelShaderSize;

	psoDesc.BlendState.AlphaToCoverageEnable = false;
	psoDesc.BlendState.IndependentBlendEnable = false;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = true;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = ::D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = ::D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp = ::D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = ::D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = ::D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = ::D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
		::D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.SampleMask = UINT_MAX;

	psoDesc.RasterizerState.FillMode = ::D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = ::D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.FrontCounterClockwise = false;
	psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	psoDesc.RasterizerState.SlopeScaledDepthBias =
		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	psoDesc.RasterizerState.DepthClipEnable = true;
	psoDesc.RasterizerState.MultisampleEnable = false;
	psoDesc.RasterizerState.AntialiasedLineEnable = false;
	psoDesc.RasterizerState.ForcedSampleCount = 0;
	psoDesc.RasterizerState.ConservativeRaster =
		::D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthWriteMask = ::D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = ::D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.DepthStencilState.FrontFace.StencilPassOp = ::D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilFailOp = ::D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp =
		::D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace = psoDesc.DepthStencilState.FrontFace;

	D3D12_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex3D, x),
		 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},

		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex3D, nx),
		 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex3D, u),
		 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(Vertex3D, col),
		 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	psoDesc.InputLayout = {layout, _countof(layout)};

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	psoDesc.SampleDesc.Count = 1;
	psoDesc.NodeMask = 1;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	auto hr = m_d3dDevice->CreateGraphicsPipelineState(
		&psoDesc, m_d3d3DPipelineState.static_uuid, m_d3d3DPipelineState.Out());
	if (D3DRIISERROR(hr, "device->CreateGraphicsPipelineState"))
	{
		FatalError("Failed to create pipeline state");
	}

	D3DRIASSERT(m_d3d3DPipelineState);

	m_d3d3DPipelineState->SetName(L"D3DRI::m_d3d3DPipelineState");
}

auto D3DRI::CreateSwapChain() -> void
{

	D3DRIASSERT(m_dxgiFactory);
	D3DRIASSERT(m_d3dCommandQueue);

	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.Width = m_windowWidth;
	desc.Height = m_windowHeight;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = false;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = kNumSwapChainFrames;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = 0;

	COMPtr<IDXGISwapChain1> swapChain{};
	auto hr = m_dxgiFactory->CreateSwapChainForHwnd(
		m_d3dCommandQueue, m_windowHandle, &desc, nullptr, nullptr,
		swapChain.Out());
	if (D3DRIISERROR(hr, "factory->CreateSwapChainForHwnd"))
	{
		FatalError("Unable to create swap chain!");
	}

	hr = swapChain->QueryInterface(m_dxgiSwapChain.static_uuid,
								   m_dxgiSwapChain.Out());
	if (D3DRIISERROR(hr, "swapChain->QueryInterface"))
	{
		FatalError("Unable to get swap chain interface!");
	}

	D3DRIASSERT(m_dxgiSwapChain);
}

auto D3DRI::CreateRenderTargets() -> void
{

	D3DRIASSERT(m_d3dDevice);
	D3DRIASSERT(m_dxgiSwapChain);

	auto rtvHandle =
		m_d3dRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto i = 0; i < kNumSwapChainFrames; i++)
	{
		auto hr = m_dxgiSwapChain->GetBuffer(
			i, m_d3dRenderTargets[i].static_uuid, m_d3dRenderTargets[i].Out());
		if (D3DRIISERROR(hr, "swapChain->GetBuffer"))
		{
			FatalError("Unable to get swap chain buffer!");
		}
		D3DRIASSERT(m_d3dRenderTargets[i]);

		m_d3dDevice->CreateRenderTargetView(m_d3dRenderTargets[i], nullptr,
											rtvHandle);
		D3DRIASSERT(m_d3dRenderTargets[i]);

		rtvHandle.ptr += m_d3dRTVDescriptorSize;

		m_d3dRenderTargets[i]->SetName(L"D3DRI::m_d3dRenderTargets");
	}

	m_frameIndex = 0;
}

auto D3DRI::CreateDepthBuffer() -> void
{

	D3DRIASSERT(m_d3dDevice);
	D3DRIASSERT(m_d3dDSVDescriptorHeap);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = m_windowWidth;
	resourceDesc.Height = m_windowHeight;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	auto hr = m_d3dDevice->CreateCommittedResource(
		&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
		m_d3dDepthBuffer.static_uuid, m_d3dDepthBuffer.Out());
	if (D3DRIISERROR(hr, "device->CreateCommittedResource"))
	{
		FatalError("Unable to create depth buffer!");
	}

	D3DRIASSERT(m_d3dDepthBuffer);

	m_d3dDepthBuffer->SetName(L"D3DRI::m_d3dDepthBuffer");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_d3dDevice->CreateDepthStencilView(
		m_d3dDepthBuffer, &dsvDesc,
		m_d3dDSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

auto D3DRI::ShouldRender() -> bool
{

	if (m_windowIsMinimized)
		return false;
	return true;
}

auto D3DRI::RenderFrame() -> void
{

	D3DRIASSERT(m_dxgiSwapChain);
	D3DRIASSERT(m_d3dDevice);

	FillAndExecuteCommandList();

	SignalD3DFence();

	auto hr = m_dxgiSwapChain->Present(0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		auto removedReason = m_d3dDevice->GetDeviceRemovedReason();
		LogWinError(LogLevel::Warning, hr, "Present");
		LogWinError(LogLevel::Warning, removedReason, "GetDeviceRemovedReason");
		DeviceLost();
	}
	else if (D3DRIISERROR(hr, "Present"))
	{
		FatalError("Unable to present!");
	}

	WaitForD3DFence();

	m_frameNumber++;
	m_frameIndex++;
	m_frameIndex %= kNumSwapChainFrames;
}

auto D3DRI::FillAndExecuteCommandList() -> void
{

	const auto queue = m_d3dCommandQueue.Get();
	const auto alloc = m_d3dCommandAllocators[m_frameIndex].Get();
	const auto target = m_d3dRenderTargets[m_frameIndex].Get();
	const auto list = m_d3dCommandList.Get();
	D3DRIASSERT(queue);
	D3DRIASSERT(alloc);
	D3DRIASSERT(target);
	D3DRIASSERT(list);

	const auto rootSig = m_d3dRootSignature.Get();
	D3DRIASSERT(rootSig);

	const auto pipelineState = m_d3d3DPipelineState.Get();
	D3DRIASSERT(pipelineState);

	const auto srvHeap = m_d3dSRVDescriptorHeap.Get();
	D3DRIASSERT(srvHeap);
	const auto rtvHeap = m_d3dRTVDescriptorHeap.Get();
	D3DRIASSERT(rtvHeap);
	const auto dsvHeap = m_d3dDSVDescriptorHeap.Get();
	D3DRIASSERT(dsvHeap);

	auto hr = alloc->Reset();
	if (D3DRIISERROR(hr, "commandAllocator->Reset"))
	{
		FatalError("Unable to reset command allocator!");
	}

	hr = list->Reset(alloc, pipelineState);
	if (D3DRIISERROR(hr, "commandList->Reset"))
	{
		FatalError("Unable to reset command list!");
	}

	list->SetGraphicsRootSignature(rootSig);

	ID3D12DescriptorHeap* heaps[] = {srvHeap};
	list->SetDescriptorHeaps(_countof(heaps), heaps);

	// list->SetGraphicsRootDescriptorTable(
	//		2, srvHeap->GetGPUDescriptorHandleForHeapStart());

	list->RSSetViewports(1, &m_viewport);
	list->RSSetScissorRects(1, &m_scissorRect);

	TransitionResource(list, target, D3D12_RESOURCE_STATE_COMMON,
					   D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += m_frameIndex * m_d3dRTVDescriptorSize;
	m_d3dCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	list->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0,
								nullptr);

	float clearColor[] = {0.f, 0.f, 0.f, 1.f};
	m_d3dCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	D3DRIRenderFrameContext frameContext{};
	frameContext.m_queue = queue;
	frameContext.m_alloc = alloc;
	frameContext.m_target = target;
	frameContext.m_list = list;
	frameContext.m_ri = this;

	frameContext.m_modelConstants.m_uvOffset[0] = 0.f;
	frameContext.m_modelConstants.m_uvOffset[1] = 0.f;
	frameContext.m_modelConstants.m_uvScale[0] = 1.f;
	frameContext.m_modelConstants.m_uvScale[1] = 1.f;

	frameContext.FillCommandList();

	TransitionResource(list, target, D3D12_RESOURCE_STATE_RENDER_TARGET,
					   D3D12_RESOURCE_STATE_PRESENT);

	hr = list->Close();
	if (D3DRIISERROR(hr, "commandList->Close"))
	{
		FatalError("Unable to close command list!");
	}

	ID3D12CommandList* lists[] = {list};
	queue->ExecuteCommandLists(_countof(lists), lists);
}

auto D3DRI::DeviceLost() -> void
{

	LogMessage(LogLevel::Warning, "DeviceLost");
	ReleaseDeviceAndResources();
	const auto mgr = GetResourceManager();
	D3DRIASSERT(mgr);
	mgr->ReleaseGPUResources();
	CreateDeviceAndResources();
	// Will be called before present:
	// And we can only get here from resize or present so this is okay
	// ValidateAndCreateGPUResources();
	LogMessage(LogLevel::Info, "Device resources recreated!");
}

auto D3DRI::ReleaseSwapChainDependantResources() -> void
{

	for (auto i = 0; i < kNumSwapChainFrames; i++)
	{
		m_d3dRenderTargets[i].Reset();
	}

	m_d3dDepthBuffer.Reset();
}

auto D3DRI::CreateSwapChainDependantResources() -> void
{

	CreateRenderTargets();
	CreateDepthBuffer();
}

auto D3DRI::ReleaseDeviceAndResources() -> void
{

	ReleaseSwapChainDependantResources();
	if (m_dxgiSwapChain)
	{
		m_dxgiSwapChain->SetFullscreenState(FALSE, nullptr);
		m_dxgiSwapChain.Reset();
	}
	m_d3d3DPipelineState.Reset();
	m_d3dSRVDescriptorHeap.Reset();
	m_d3dDSVDescriptorHeap.Reset();
	m_d3dRTVDescriptorHeap.Reset();
	m_d3dRootSignature.Reset();
	m_d3dFence.Reset();
	m_d3dCommandList.Reset();
	for (auto& allocator : m_d3dCommandAllocators)
	{
		allocator.Reset();
	}
	m_d3dCommandQueue.Reset();
	m_d3dDevice.Reset();
}

auto D3DRI::CreateDeviceAndResources() -> void
{

	CreateDevice();

	CreateCommandQueue();

	CreateCommandAllocators();

	CreateCommandList();

	CreateFence();

	CreateRootSignature();

	CreateDescriptorHeaps();

	CreatePipelineStates();

	CreateSwapChain();

	CreateSwapChainDependantResources();
}

auto D3DRI::TransitionResource(ID3D12GraphicsCommandList* list,
							   ID3D12Resource* resource,
							   D3D12_RESOURCE_STATES before,
							   D3D12_RESOURCE_STATES after) -> void
{

	D3DRIASSERT(list);
	D3DRIASSERT(resource);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;

	list->ResourceBarrier(1, &barrier);
}

auto D3DRI::CreateBufferResource(const D3D12_RESOURCE_DESC& desc,
								 D3D12_HEAP_TYPE heapType,
								 D3D12_RESOURCE_STATES initialState)
	-> COMPtr<ID3D12Resource>
{

	D3DRIASSERT(m_d3dDevice);
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = heapType;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	COMPtr<ID3D12Resource> resource{};

	auto hr = m_d3dDevice->CreateCommittedResource(
		&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, initialState, nullptr,
		resource.static_uuid, resource.InOut());
	if (IsError(hr, "CreateCommittedResource"))
	{
		FatalError("CreateCommittedResource failed");
	}

	D3DRIASSERT(resource);

	return resource;
}

auto D3DRI::Update(float deltaTime) -> void
{
	m_time += deltaTime;
}

auto D3DRI::GetResourceManager() -> D3DRIResourceManager*
{
	return m_resourceManager;
}

auto D3DRI::ValidateAndCreateGPUResources() -> void
{
	D3DRIASSERT(m_resourceManager);

	if (m_resourceManager->GetNumResourcesNotCreated() > 0)
	{
		// Shouldn't be needed as we waited in RenderFrame
		// SignalD3DFence();
		// WaitForD3DFence();

		D3DRIASSERT(m_d3dDevice);
		D3DRIASSERT(m_d3dCommandList);
		D3DRIASSERT(m_d3dCommandQueue);
		const auto alloc = m_d3dCommandAllocators[m_frameIndex];
		D3DRIASSERT(alloc);

		auto hr = m_d3dCommandList->Reset(alloc, m_d3d3DPipelineState);
		if (D3DRIISERROR(hr, "commandList->Reset"))
		{
			FatalError("Unable to reset command list!");
		}

		D3DRIBatchResourceCreationContext ctx{};
		ctx.m_ri = this;
		ctx.m_device = m_d3dDevice;
		ctx.m_commandList = m_d3dCommandList;
		ctx.m_commandAllocator = alloc;
		ctx.m_commandQueue = m_d3dCommandQueue;

		m_resourceManager->CreateGPUResources(ctx);

		hr = m_d3dCommandList->Close();
		if (D3DRIISERROR(hr, "commandList->Close"))
		{
			FatalError("Unable to close command list!");
		}

		ID3D12CommandList* lists[] = {m_d3dCommandList};
		m_d3dCommandQueue->ExecuteCommandLists(_countof(lists), lists);

		SignalD3DFence();
		WaitForD3DFence();

		
	}
}

auto D3DRI::CreateGenericResources() -> void
{
	if (!m_genericCubeResource)
	{
		Vertex3D vertices[] = {
			// Z = 1
			{-1.f, 1.f, 1.0f, 0.f, 0.f, -1.f, 0.f, 1.f, 0xff0000ff},
			{1.f, 1.f, 1.0f, 0.f, 0.f, -1.f, 1.f, 1.f, 0xff00ff00},
			{-1.f, -1.f, 1.0f, 0.f, 0.f, -1.f, 0.f, 0.f, 0xffff0000},
			{-1.f, -1.f, 1.0f, 0.f, 0.f, -1.f, 0.f, 0.f, 0xffff0000},
			{1.f, 1.f, 1.0f, 0.f, 0.f, -1.f, 1.f, 1.f, 0xff00ff00},
			{1.f, -1.f, 1.0f, 0.f, 0.f, 1.f, 1.f, 0.f, 0xff0000ff},
			// Z = -1
			{-1.f, 1.f, -1.0f, 0.f, 0.f, 1.f, 0.f, 1.f, 0xff0000ff},
			{1.f, 1.f, -1.0f, 0.f, 0.f, 1.f, 1.f, 1.f, 0xff00ff00},
			{-1.f, -1.f, -1.0f, 0.f, 0.f, 1.f, 0.f, 0.f, 0xffff0000},
			{-1.f, -1.f, -1.0f, 0.f, 0.f, 1.f, 0.f, 0.f, 0xffff0000},
			{1.f, 1.f, -1.0f, 0.f, 0.f, 1.f, 1.f, 1.f, 0xff00ff00},
			{1.f, -1.f, -1.0f, 0.f, 0.f, 1.f, 1.f, 0.f, 0xff0000ff},
			// Y = 1
			{-1.f, 1.f, -1.0f, 0.f, -1.f, 0.f, 0.f, 1.f, 0xff0000ff},
			{1.f, 1.f, -1.0f, 0.f, -1.f, 0.f, 1.f, 1.f, 0xff00ff00},
			{-1.f, 1.f, 1.0f, 0.f, -1.f, 0.f, 0.f, 0.f, 0xffff0000},
			{-1.f, 1.f, 1.0f, 0.f, -1.f, 0.f, 0.f, 0.f, 0xffff0000},
			{1.f, 1.f, -1.0f, 0.f, -1.f, 0.f, 1.f, 1.f, 0xff00ff00},
			{1.f, 1.f, 1.0f, 0.f, -1.f, 0.f, 1.f, 0.f, 0xff0000ff},
			// Y = -1
			{-1.f, -1.f, -1.0f, 0.f, 1.f, 0.f, 0.f, 1.f, 0xff0000ff},
			{1.f, -1.f, -1.0f, 0.f, 1.f, 0.f, 1.f, 1.f, 0xff00ff00},
			{-1.f, -1.f, 1.0f, 0.f, 1.f, 0.f, 0.f, 0.f, 0xffff0000},
			{-1.f, -1.f, 1.0f, 0.f, 1.f, 0.f, 0.f, 0.f, 0xffff0000},
			{1.f, -1.f, -1.0f, 0.f, 1.f, 0.f, 1.f, 1.f, 0xff00ff00},
			{1.f, -1.f, 1.0f, 0.f, 1.f, 0.f, 1.f, 0.f, 0xff0000ff},
			// X = 1
			{1.f, -1.f, -1.0f, -1.f, 0.f, 0.f, 0.f, 1.f, 0xff0000ff},
			{1.f, 1.f, -1.0f, -1.f, 0.f, 0.f, 1.f, 1.f, 0xff00ff00},
			{1.f, -1.f, 1.0f, -1.f, 0.f, 0.f, 0.f, 0.f, 0xffff0000},
			{1.f, -1.f, 1.0f, -1.f, 0.f, 0.f, 0.f, 0.f, 0xffff0000},
			{1.f, 1.f, -1.0f, -1.f, 0.f, 0.f, 1.f, 1.f, 0xff00ff00},
			{1.f, 1.f, 1.0f, -1.f, 0.f, 0.f, 1.f, 0.f, 0xff0000ff},
			// X = -1
			{-1.f, -1.f, -1.0f, 1.f, 0.f, 0.f, 0.f, 1.f, 0xff0000ff},
			{-1.f, 1.f, -1.0f, 1.f, 0.f, 0.f, 1.f, 1.f, 0xff00ff00},
			{-1.f, -1.f, 1.0f, 1.f, 0.f, 0.f, 0.f, 0.f, 0xffff0000},
			{-1.f, -1.f, 1.0f, 1.f, 0.f, 0.f, 0.f, 0.f, 0xffff0000},
			{-1.f, 1.f, -1.0f, 1.f, 0.f, 0.f, 1.f, 1.f, 0xff00ff00},
			{-1.f, 1.f, 1.0f, 1.f, 0.f, 0.f, 1.f, 0.f, 0xff0000ff},
		};
		m_genericCubeResource =
			GetResourceManager()->CreateResource<D3DRIVertexBufferResource>(
				reinterpret_cast<atd::uint8_t*>(vertices), sizeof vertices,
				sizeof *vertices);
	}
}
