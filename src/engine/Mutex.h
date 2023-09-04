#pragma once

#include "../Windows/Win.h"

struct Mutex
{
	inline Mutex()
	{
		m_mutex = CreateMutexA(nullptr, FALSE, nullptr);
	}

	inline ~Mutex()
	{
		CloseHandle(m_mutex);
	}

	inline void lock()
	{
		WaitForSingleObject(m_mutex, INFINITE);
	}

	inline void unlock()
	{
		ReleaseMutex(m_mutex);
	}

	HANDLE m_mutex{};
};