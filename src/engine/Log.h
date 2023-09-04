#pragma once

#include "../Windows/Win.h"

enum class LogLevel
{
	Info,
	Warning,
	Error,
	Fatal
};

auto LogMessage(LogLevel level, const char* msg) -> void;
auto LogWinError(LogLevel level, HRESULT hr, const char* msg) -> void;