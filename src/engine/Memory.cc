#include "Memory.h"

HANDLE g_heapHandle{};

auto AllocateMemory(size_t size, size_t align) -> void*
{
	// Can only allocate 16 byte aligned memory
	// therefore align must be either 0, 1, 2, 4, 8, or a multiple of 16
	if (align != 0 && align != 1 && align != 2 && align != 4 && align != 8 &&
		align % 16 != 0)
	{
		ASSERTCHECK(false);
		return nullptr;
	}

	const auto ptr = HeapAlloc(g_heapHandle, HEAP_ZERO_MEMORY, size);
	ASSERTCHECK(ptr != nullptr);
	return ptr;
}

auto FreeMemory(void* ptr, size_t) -> void
{
	if (ptr)
		HeapFree(g_heapHandle, 0, ptr);
}

auto operator new(size_t size) -> void* {
	return AllocateMemory(size, 16);
}

auto operator new[](size_t size) -> void* {
	return AllocateMemory(size, 16);
}

auto operator delete(void* ptr) -> void {
	FreeMemory(ptr, 0);
}

auto operator delete[](void* ptr) -> void {
	FreeMemory(ptr, 0);
}

auto operator delete(void* ptr, atd::size_t size) noexcept -> void {
	FreeMemory(ptr, size);
}

auto MemoryInitialize() -> void
{
	g_heapHandle =
		HeapCreate(HEAP_CREATE_ENABLE_EXECUTE | HEAP_CREATE_ALIGN_16, 0, 0);
	LogMessage(LogLevel::Info, "Initialized memory");
}

auto MemoryShutdown() -> void
{
	HeapDestroy(g_heapHandle);
	LogMessage(LogLevel::Info, "Shutdown memory");
}
