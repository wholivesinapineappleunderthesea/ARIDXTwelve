#pragma once
#include "Common.h"

struct Vertex3DNoColour
{
	float x{}, y{}, z{};
	float nx{}, ny{}, nz{};
	float u{}, v{};
};

struct Vertex3D
{
	float x{}, y{}, z{};
	float nx{}, ny{}, nz{};
	float u{}, v{};
	atd::uint32_t col{};
};