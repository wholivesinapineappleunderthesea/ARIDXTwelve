#pragma once

#define ASSERTCHECK(x)                                                         \
	do                                                                         \
	{                                                                          \
		if (!(x))                                                              \
		{                                                                      \
			atd::fatal_error();                                                \
		}                                                                      \
	} while (0)

namespace atd
{
	using uint8_t = unsigned char;
	static_assert(sizeof(uint8_t) == 1, "uint8_t is not 1 byte");
	using uint16_t = unsigned short;
	static_assert(sizeof(uint16_t) == 2, "uint16_t is not 2 bytes");
	using uint32_t = unsigned int;
	static_assert(sizeof(uint32_t) == 4, "uint32_t is not 4 bytes");
	using uint64_t = unsigned long long;
	static_assert(sizeof(uint64_t) == 8, "uint64_t is not 8 bytes");

	using int8_t = signed char;
	static_assert(sizeof(int8_t) == 1, "int8_t is not 1 byte");
	using int16_t = signed short;
	static_assert(sizeof(int16_t) == 2, "int16_t is not 2 bytes");
	using int32_t = signed int;
	static_assert(sizeof(int32_t) == 4, "int32_t is not 4 bytes");
	using int64_t = signed long long;
	static_assert(sizeof(int64_t) == 8, "int64_t is not 8 bytes");

	using float32_t = float;
	static_assert(sizeof(float32_t) == 4, "float32_t is not 4 bytes");
	using float64_t = double;
	static_assert(sizeof(float64_t) == 8, "float64_t is not 8 bytes");

	using byte_t = uint8_t;

	using size_t = uint64_t;
	using ssize_t = int64_t;

	using ptrdiff_t = ssize_t;

	using uintptr_t = uint64_t;
	using intptr_t = int64_t;

	using uintmax_t = uint64_t;
	using intmax_t = int64_t;

	using nullptr_t = decltype(nullptr);

	[[noreturn]] inline auto fatal_error() -> void
	{
		__debugbreak();
	}

	// strlen
	inline auto strlen(const char* str) -> size_t
	{
		size_t len = 0;
		while (*str++)
			++len;
		return len;
	}
	// strcpy
	inline auto strcpy(char* to, const char* from) -> char*
	{
		auto to_ = to;
		while (*to++ = *from++)
			;
		return to_;
	}
	// strcat
	inline auto strcat(char* to, const char* from) -> char*
	{
		auto to_ = to;
		while (*to)
			++to;
		while (*to++ = *from++)
			;
		return to_;
	}
	// strcat
	inline auto strcat(char* to, const char* from, size_t size) -> char*
	{
		auto to_ = to;
		while (*to)
			++to;
		while (size-- && (*to++ = *from++))
			;
		return to_;
	}
	// memmove
	inline auto memmove(void* to, const void* from, size_t size) -> void*
	{
		auto to_ = reinterpret_cast<char*>(to);
		auto from_ = reinterpret_cast<const char*>(from);
		if (to_ < from_)
		{
			while (size--)
				*to_++ = *from_++;
		}
		else
		{
			to_ += size;
			from_ += size;
			while (size--)
				*--to_ = *--from_;
		}
		return to;
	}
	// memcpy
	inline auto memcpy(void* to, const void* from, size_t size) -> void*
	{
		auto to_ = reinterpret_cast<char*>(to);
		auto from_ = reinterpret_cast<const char*>(from);
		while (size--)
			*to_++ = *from_++;
		return to;
	}

	// false_type
	struct false_type
	{
		static constexpr bool value = false;
		using value_type = bool;
		using type = false_type;
		constexpr operator bool() const noexcept
		{
			return false;
		}
		constexpr bool operator()() const noexcept
		{
			return false;
		}
	};

	// true_type
	struct true_type
	{
		static constexpr bool value = true;
		using value_type = bool;
		using type = true_type;
		constexpr operator bool() const noexcept
		{
			return true;
		}
		constexpr bool operator()() const noexcept
		{
			return true;
		}
	};

	// remove_reference
	template <class T> struct remove_reference
	{
		using type = T;
	};
	template <class T> struct remove_reference<T&>
	{
		using type = T;
	};
	template <class T> struct remove_reference<T&&>
	{
		using type = T;
	};
	template <class T>
	using remove_reference_t = typename remove_reference<T>::type;
	// is_rvalue_reference
	template <class T> struct is_rvalue_reference : false_type
	{
	};
	template <class T> struct is_rvalue_reference<T&&> : true_type
	{
	};
	template <class T>
	constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;
	// is_lvalue_reference
	template <class T> struct is_lvalue_reference : false_type
	{
	};
	template <class T> struct is_lvalue_reference<T&> : true_type
	{
	};
	template <class T>
	constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

	// add_rvalue_reference
	template <class T> struct add_rvalue_reference
	{
		using type = T&&;
	};
	template <class T> struct add_rvalue_reference<T&>
	{
		using type = T&;
	};
	template <class T>
	using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

	// forward
	template <class T>
	inline T&& forward(typename remove_reference<T>::type& t) noexcept
	{
		return static_cast<T&&>(t);
	}
	template <class T>
	inline T&& forward(typename remove_reference<T>::type&& t) noexcept
	{
		static_assert(!is_lvalue_reference<T>::value, "bad forward call");
		return static_cast<T&&>(t);
	}

	// move
	template <class T>
	inline typename remove_reference<T>::type&& move(T&& t) noexcept
	{
		return static_cast<typename remove_reference<T>::type&&>(t);
	}

	// always_false
	template <typename T> struct always_false : false_type
	{
	};

	// declval
	template <class T>
	inline auto declval() noexcept -> add_rvalue_reference_t<T>
	{
		static_assert(always_false<T>, "not allowed for evaluated context");
	}

	// swap
	template <class T> inline void swap(T& a, T& b) noexcept
	{
		T tmp = move(a);
		a = move(b);
		b = move(tmp);
	}

	// derived_from
	template <class _Derived, class _Base>
	concept derived_from =
		__is_base_of(_Base, _Derived) &&
		__is_convertible_to(const volatile _Derived*, const volatile _Base*);

	// is_same
	template <class T, class U> struct is_same
	{
		static constexpr bool value = false;
	};
	template <class T> struct is_same<T, T>
	{
		static constexpr bool value = true;
	};
	template <class T, class U> constexpr bool is_same_v = is_same<T, U>::value;

	// same_as
	template <class T, class U>
	concept same_as = is_same_v<T, U> && is_same_v<U, T>;

	// convertible_to
	template <class From, class To>
	concept convertible_to = __is_convertible_to(From, To) &&
							 requires { static_cast<To>(declval<From>()); };

}; // namespace atd

#include "Array.h"
#include "Log.h"
#include "Math.h"
#include "Memory.h"
#include "Mutex.h"
#include "String.h"
