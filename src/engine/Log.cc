#include "Log.h"
#include "Memory.h"
#include "String.h"

auto LogMessage(LogLevel level, const char* msg) -> void
{
	const char* levelStr{};
	switch (level)
	{
	case LogLevel::Info:
		levelStr = "[INFO]";
		break;
	case LogLevel::Warning:
		levelStr = "[WARNING]";
		break;
	case LogLevel::Error:
		levelStr = "[ERROR]";
		break;
	case LogLevel::Fatal:
		levelStr = "[FATAL]";
		break;
	}

	const auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (handle != INVALID_HANDLE_VALUE)
	{
		DWORD written{};
		if (levelStr)
		{
			const auto len = static_cast<DWORD>(atd::strlen(levelStr));
			WriteConsoleA(handle, levelStr, len, &written, nullptr);
			WriteConsoleA(handle, " ", 1, &written, nullptr);
		}
		const auto len = static_cast<DWORD>(atd::strlen(msg));
		WriteConsoleA(handle, msg, len, &written, nullptr);
		WriteConsoleA(handle, "\n", 1, &written, nullptr);
	}
}

auto LogWinError(LogLevel level, HRESULT hr, const char* msg) -> void
{
	(void)level;
	(void)hr;
	(void)msg;

	// Use FormatMessageA to get the error message for the error code
	// and append it to the message string.

	LPSTR comErrMsg{};
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
					   FORMAT_MESSAGE_IGNORE_INSERTS,
				   nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				   reinterpret_cast<LPTSTR>(&comErrMsg), 0, nullptr);
	if (comErrMsg != nullptr)
	{
		size_t comErrTxtLen = atd::strlen(comErrMsg);
		size_t sz = atd::strlen(msg) + comErrTxtLen + 16;

		char* buf = static_cast<char*>(AllocateMemory(sz));
		atd::strcpy(buf, msg);
		atd::strcat(buf, "\n\t");
		atd::strcat(buf, comErrMsg, comErrTxtLen);
		LogMessage(level, buf);
		FreeMemory(buf);

		LocalFree(comErrMsg);
	}
}