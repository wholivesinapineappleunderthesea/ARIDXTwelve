#pragma once
#include "../engine/Common.h"
#include "Win.h"
#include <Unknwn.h>

#define COMPTRSETPTRATOMIC(pptr, value)                                        \
	_InterlockedExchangePointer((PVOID*)pptr, (PVOID)value)

template <typename T>
concept IUnknownDerived = atd::derived_from<T, IUnknown>;

inline auto COMPtrAddRef(IUnknown* ptr) noexcept -> void
{
	if (ptr)
	{
		ptr->AddRef();
	}
}
inline auto COMPtrRelease(IUnknown* ptr) noexcept -> void
{
	if (ptr)
	{
		ptr->Release();
	}
}

template <IUnknownDerived T> struct COMPtrOut
{
	inline COMPtrOut() = delete;
	inline COMPtrOut(const COMPtrOut&) = delete;
	inline COMPtrOut(IUnknown** ptr) noexcept : m_pptr(ptr)
	{
		if (*m_pptr)
		{
			COMPtrRelease(*m_pptr);
			COMPTRSETPTRATOMIC(m_pptr, nullptr);
		}
	}
	inline ~COMPtrOut()
	{
		if (m_outPtr)
		{
			COMPTRSETPTRATOMIC(m_pptr, m_outPtr);
		}
	}
	inline operator T**() noexcept
	{
		return reinterpret_cast<T**>(&m_outPtr);
	}
	inline operator void**() noexcept
	{
		return reinterpret_cast<void**>(&m_outPtr);
	}

  private:
	IUnknown** m_pptr{};
	IUnknown* m_outPtr{};
};

template <IUnknownDerived T> struct COMPtrInOut
{
	inline COMPtrInOut() = delete;
	inline COMPtrInOut(const COMPtrInOut&) = delete;
	inline COMPtrInOut(IUnknown** ptr) noexcept : m_pptr(ptr), m_previous(*ptr)
	{
	}
	inline ~COMPtrInOut()
	{
		if (m_previous)
		{
			if (m_previous != *m_pptr)
			{
				COMPtrRelease(m_previous);
			}
		}
	}
	inline operator T**() const noexcept
	{
		return reinterpret_cast<T**>(m_pptr);
	}
	inline operator void**() const noexcept
	{
		return reinterpret_cast<void**>(m_pptr);
	}

  private:
	IUnknown** m_pptr{};
	IUnknown* m_previous{};
};

template <typename T> struct COMPtr
{
	static inline auto static_uuid = __uuidof(T);
	using type = T;
	using pointer = T*;
	using reference = T&;

	// Very similar to atd::inout_ptr()
	inline auto InOut() noexcept -> COMPtrInOut<T>
	{
		return COMPtrInOut<T>{&_ptr};
	}
	// Very similar to atd::out_ptr()
	inline auto Out() noexcept -> COMPtrOut<T>
	{
		return COMPtrOut<T>{&_ptr};
	}
	// Ptr operations:
	inline auto operator->() noexcept -> pointer
	{
		return Get();
	}
	inline auto operator->() const noexcept -> const pointer
	{
		return Get();
	}
	inline auto operator*() noexcept -> reference
	{
		return *Get();
	}
	inline auto operator*() const noexcept -> const reference
	{
		return *Get();
	}
	inline operator pointer() noexcept
	{
		return Get();
	}
	inline operator const pointer() const noexcept
	{
		return Get();
	}

	// Boolean operations:
	inline operator bool() const noexcept
	{
		return Get() != nullptr;
	}
	inline auto operator!() const noexcept -> bool
	{
		return Get() == nullptr;
	}
	inline auto operator==(atd::nullptr_t) const noexcept -> bool
	{
		return Get() == nullptr;
	}
	inline auto operator!=(atd::nullptr_t) const noexcept -> bool
	{
		return Get() != nullptr;
	}

	// Comparison operations:
	inline auto operator==(const COMPtr& other) const noexcept -> bool
	{
		return _ptr == other._ptr;
	}
	inline auto operator!=(const COMPtr& other) const noexcept -> bool
	{
		return _ptr != other._ptr;
	}

	// Constructors:
	// Constructs with nullptr
	inline COMPtr() noexcept
	{
		COMPTRSETPTRATOMIC(&_ptr, nullptr);
	}
	// Constructs with nullptr
	inline COMPtr(atd::nullptr_t) noexcept
	{
		COMPTRSETPTRATOMIC(&_ptr, nullptr);
	}
	// Constructs with ref'd ptr, requires that T2 publicly derives from or is
	// T
	inline COMPtr(T* ptr) noexcept
	{
		COMPTRSETPTRATOMIC(&_ptr, ptr);
	}

	// Copy constructor from same T
	inline COMPtr(const COMPtr& other) noexcept
	{
		COMPTRSETPTRATOMIC(&_ptr, other.Get());
		AddRef();
	}
	// Move constructor from same T
	inline COMPtr(COMPtr&& other) noexcept
	{
		COMPTRSETPTRATOMIC(&_ptr, other.Get());
		COMPTRSETPTRATOMIC(&other._ptr, nullptr);
	}

	// Destructor:
	inline ~COMPtr()
	{
		Release();
	}

	// Assigns with a nullptr
	inline auto operator=(atd::nullptr_t)
	{
		Reset();
	}
	// Assigns with a ref'd ptr, requires that T2 publicly derives from or is T
	inline auto operator=(T* ptr) noexcept
	{
		Reset(ptr);
	}
	// Assign copy from same T
	inline auto operator=(const COMPtr& other) noexcept
	{
		Reset(other.Get());
		AddRef();
	}
	// Assign move from same T
	inline auto operator=(COMPtr&& other) noexcept
	{
		Reset(other.Get());
		COMPTRSETPTRATOMIC(&other._ptr, nullptr);
	}

	// Member functions:
	inline auto Get() noexcept -> pointer
	{
		return reinterpret_cast<pointer>(_ptr);
	}
	inline auto Get() const noexcept -> const pointer
	{
		return reinterpret_cast<const pointer>(_ptr);
	}
	inline auto Reset(T* ptr = nullptr) -> void
	{
		Release();
		COMPTRSETPTRATOMIC(&_ptr, ptr);
	}
	inline auto Release() -> void
	{
		if (_ptr)
		{
			COMPtrRelease(_ptr);
			COMPTRSETPTRATOMIC(&_ptr, nullptr);
		}
	}
	inline auto AddRef() -> void
	{
		if (_ptr)
		{
			COMPtrAddRef(_ptr);
		}
	}
	IUnknown* _ptr{};
};

#undef COMPTRSETPTRATOMIC