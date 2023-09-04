#pragma once

#include "Common.h"

// Allocates zero-initialized heap memory
auto AllocateMemory(atd::size_t size, atd::size_t align = 16) -> void*;
// Frees memory allocated by AllocateMemory
auto FreeMemory(void* ptr, atd::size_t sizeIfKnown = 0) -> void;

auto MemoryInitialize() -> void;
auto MemoryShutdown() -> void;

// Global new and delete operators
auto operator new(atd::size_t size) -> void*;
auto operator new[](atd::size_t size) -> void*;
auto operator delete(void* ptr) noexcept -> void;
auto operator delete[](void* ptr) noexcept -> void;
auto operator delete(void* ptr, atd::size_t size) noexcept -> void;