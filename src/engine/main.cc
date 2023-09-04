#include "Common.h"
#include "GlobalWindowData.h"
#include "renderer/D3DRI.h"
#include "renderer/D3DRIResourceManager.h"

#include "World/WorldManager.h"
#include "World/World1.h"

auto UpdateAll([[maybe_unused]] float dt) -> void
{
	if (g_D3DRI)
	{
		g_D3DRI->Update(dt);
	}
	if (g_worldManager)
	{
		g_worldManager->Lock();
		g_worldManager->Update(dt);
		g_worldManager->Unlock();
	}
}

static inline constexpr auto kNumMaxUpdatesPerSecond = 200;
static inline constexpr auto kNumMinUpdateTime =
	1.f / static_cast<float>(kNumMaxUpdatesPerSecond);
bool g_bShouldUpdateAll{};
bool g_bShutdownUpdateThread{};
auto UpdateThread() -> void
{
	LARGE_INTEGER lastTime{};
	LARGE_INTEGER freq{};
	while (!QueryPerformanceFrequency(&freq))
	{
		Sleep(static_cast<DWORD>(kNumMinUpdateTime * 1000.f));
	}
	while (!QueryPerformanceCounter(&lastTime))
	{
		Sleep(static_cast<DWORD>(kNumMinUpdateTime * 1000.f));
	}
	while (!g_bShutdownUpdateThread)
	{
		if (!g_bShouldUpdateAll)
		{
			Sleep(static_cast<DWORD>(kNumMinUpdateTime * 1000.f));
			continue;
		}
		LARGE_INTEGER time{};
		if (QueryPerformanceCounter(&time))
		{
			const auto deltaTime =
				static_cast<float>(time.QuadPart - lastTime.QuadPart) /
				static_cast<float>(freq.QuadPart);
			if (deltaTime >= kNumMinUpdateTime)
			{
				UpdateAll(deltaTime);
				lastTime = time;
			}
			else
			{
				const auto secsToSleep = kNumMinUpdateTime - deltaTime;
				Sleep(static_cast<DWORD>(secsToSleep * 1000.f));
			}
		}
	}

	LogMessage(LogLevel::Info, "Update thread shutting down");
}

auto __stdcall mainCRTStartup() -> void
{
	MemoryInitialize();
	LogMessage(LogLevel::Info, "Starting!");
	if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
	{
		g_windowData = new GlobalWindowData{};
		g_D3DRI = new D3DRI{};
		g_worldManager = new WorldManager{};
		g_worldManager->Lock();
		g_worldManager->LoadWorld<World1>();
		g_worldManager->Unlock();

		const auto update =
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)UpdateThread, 0, 0, 0);
		if (!update)
		{
			LogWinError(LogLevel::Fatal, GetLastError(),
						"Failed to create update thread");
		}
		else
		{
			if (g_D3DRI->CreateWindowAndContext())
			{
				while (!g_D3DRI->CheckMessageQueue())
				{
					g_bShouldUpdateAll = true;
					g_D3DRI->ValidateAndCreateGPUResources();
					g_D3DRI->RenderFrame();
				}
			}
			g_bShouldUpdateAll = false;
			g_bShutdownUpdateThread = true;

			// Wait for update thread to shutdown
			WaitForSingleObject(update, INFINITE);
			CloseHandle(update);
		}
		delete g_worldManager;
		delete g_D3DRI;
		delete g_windowData;
		CoUninitialize();
	}
	MemoryShutdown();

	LogMessage(LogLevel::Info, "Shutting down!");

	ExitProcess(0);
}