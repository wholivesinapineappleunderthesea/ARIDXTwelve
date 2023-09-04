#pragma once

#include "Common.h"

template <typename TRet, typename... TArgs> struct GlobalWindowCallbackEntry
{
	using CallbackT = TRet (*)(void*, TArgs...);

	inline GlobalWindowCallbackEntry(void* ctx, CallbackT callback)
		: m_ctx(ctx), m_callback(callback)
	{
	}

	template <typename... TArgsFwdRef>
	inline auto operator()(TArgsFwdRef&&... args) const -> TRet
	{
		return m_callback(m_ctx, std::forward<TArgsFwdRef>(args)...);
	}

	void* m_ctx{};
	CallbackT m_callback{};
};

struct GlobalWindowCallbacks
{
	Array<GlobalWindowCallbackEntry<void, atd::int32_t, atd::int32_t>>
		m_onMouseMove{};
	Array<GlobalWindowCallbackEntry<void, atd::uint32_t, atd::uint32_t>>
		m_onWindowResize{};

	inline auto Lock() -> void
	{
		m_mutex.lock();
	}
	inline auto Unlock() -> void
	{
		m_mutex.unlock();
	}
	Mutex m_mutex{};
};

struct GlobalWindowData
{
	atd::uint32_t m_width{};
	atd::uint32_t m_height{};
	float m_aspectRatio{};
	bool m_isMinimized{};
	atd::int32_t m_mouseX{};
	atd::int32_t m_mouseY{};
	atd::uint8_t m_keyStates[256]{};

	GlobalWindowCallbacks m_callbacks{};
};
inline GlobalWindowData* g_windowData{};