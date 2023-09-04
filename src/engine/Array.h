#pragma once

#include "Common.h"

#include <new>

template <typename T> struct Array
{
	using value_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using size_type = size_t;
	using difference_type = atd::ptrdiff_t;
	using iterator = T*;
	using const_iterator = const T*;
	using small_size_type = atd::uint32_t;

	// data ptr
	T* m_Data;
	// count
	small_size_type m_Size;
	// reserved count
	small_size_type m_Reserved : 29;
	atd::uint8_t m_MemoryOwner : 1;
	atd::uint8_t m_ObjectOwner : 1;
	atd::uint8_t m_UNUSED : 1;

	inline Array()
	{
		m_Data = nullptr;
		m_Size = 0;
		m_Reserved = 0;
		m_MemoryOwner = 1;
		m_ObjectOwner = 1;
		m_UNUSED = 0;
	}

	inline Array(size_t count)
	{
		m_Data = InternalAlloc(count);
		m_Size = 0;
		m_Reserved = static_cast<small_size_type>(count);
		m_MemoryOwner = 1;
		m_ObjectOwner = 1;
		m_UNUSED = 0;
	}

	Array(const Array& other) = delete;
	Array(Array&& other) = default;

	// Returned array DOES own objects and memory.
	// Members are copied with the copy constructor.
	inline auto ExplicitCopy() const -> Array<T>
	{
		Array<T> ret{};
		if (m_Data)
		{
			ret.m_Data = InternalAlloc(Capacity());
			for (small_size_type i{}; i < m_Size; i++)
			{
				new (ret.m_Data + i) T{m_Data[i]};
			}
		}
		ret.m_Size = m_Size;
		ret.m_Reserved = m_Reserved;
		ret.m_MemoryOwner = 1;
		ret.m_ObjectOwner = 1;
		ret.m_UNUSED = 0;
		return ret;
	}

	// Returned array may or may not own memory and objects.
	inline auto ExplicitMove() -> Array<T>
	{
		Array<T> ret{};
		ret.m_Data = m_Data;
		ret.m_Size = m_Size;
		ret.m_Reserved = m_Reserved;
		ret.m_MemoryOwner = m_MemoryOwner;
		ret.m_ObjectOwner = m_ObjectOwner;
		ret.m_UNUSED = m_UNUSED;
		m_Data = nullptr;
		m_Size = 0;
		m_Reserved = 0;
		m_MemoryOwner = 1;
		m_ObjectOwner = 1;
		m_UNUSED = 0;
		return ret;
	}

	inline auto Size() const -> size_type
	{
		return static_cast<size_type>(m_Size);
	}
	inline auto Capacity() const -> size_type
	{
		return Size() + static_cast<size_type>(m_Reserved);
	}

	inline auto Reserve(size_type new_cap)
	{
		if (new_cap > Capacity())
		{
			// New memory is needed.
			pointer next = InternalAlloc(new_cap);
			// Copy the data.
			if (m_Data)
			{
				atd::memcpy(next, m_Data, m_Size * sizeof(T));
				// Delete old memory.
				if (m_MemoryOwner)
				{
					InternalFree(m_Data);
				}
			}
			m_Data = next;
			m_Reserved = new_cap - m_Size;
			m_MemoryOwner = 1;
		}
	}
	inline auto ShrinkToFit()
	{
		if (m_Reserved)
		{
			pointer next = InternalAlloc(m_Size);
			// Copy the data.
			atd::memcpy(next, m_Data, m_Size * sizeof(T));
			// Delete old memory.
			if (m_MemoryOwner)
			{
				InternalFree(m_Data);
			}
			m_Data = next;
			m_Reserved = 0;
			m_MemoryOwner = 1;
		}
	}

	inline auto Clear() -> void
	{
		if (m_ObjectOwner)
		{
			for (small_size_type i{}; i < m_Size; i++)
			{
				(m_Data + i)->~T();
			}
		}
		if (m_MemoryOwner)
		{
			InternalFree(m_Data);
		}
		m_Data = nullptr;
		m_Size = 0;
		m_Reserved = 0;
		m_MemoryOwner = 1;
		m_ObjectOwner = 1;
		m_UNUSED = 0;
	}

	inline auto InternalGetSpaceForInsert(iterator where = nullptr) -> iterator
	{
		// if we have no space left
		if (Size() == Capacity())
		{
			Reserve(Capacity() + 1);
		}
		// if where is nullptr we insert at the end
		if (where == nullptr)
		{
			where = end();
		}
		// move the data
		atd::memmove(reinterpret_cast<void*>(where + 1),
					 reinterpret_cast<void*>(where),
					 (end() - where) * sizeof(T));
		m_Size++;
		return where;
	}

	template <typename V>
	inline auto Insert(V&& value, iterator where = nullptr) -> iterator
	{
		const auto it = InternalGetSpaceForInsert(where);
		new (it) T{atd::forward<V>(value)};
		return it;
	}
	template <typename V>
	inline auto Insert(const V& value, iterator where = nullptr) -> iterator
	{
		const auto it = InternalGetSpaceForInsert(where);
		new (it) T{value};
		return it;
	}
	inline auto Erase(iterator where) -> iterator
	{
		if (m_ObjectOwner)
		{
			(where)->~T();
		}
		// Erases one element
		// Move elements after to the erased element
		const auto next = where + 1;
		if (next != end())
		{
			const auto cnt = end() - next;
			atd::memmove(reinterpret_cast<void*>(where),
						 reinterpret_cast<void*>(next), cnt * sizeof(T));
		}
		m_Size--;
		m_Reserved++;
		return where;
	}
	inline auto EraseMany(iterator first, iterator last) -> iterator
	{
		if (m_ObjectOwner)
		{
			for (auto it = first; it != last; it++)
			{
				(it)->~T();
			}
		}
		// Erases many elements
		// Move elements after to the erased elements
		if (last != end())
		{
			const auto cnt = end() - last;
			atd::memmove(reinterpret_cast<void*>(first),
						 reinterpret_cast<void*>(last), cnt * sizeof(T));
		}
		m_Size -= last - first;
		m_Reserved += last - first;
		return first;
	}
	inline auto Resize(size_type count, const value_type& value = value_type{})
	{
		if (m_Size > count)
		{
			if (m_ObjectOwner)
			{
				for (size_type i = count; i < m_Size; i++)
				{
					(begin() + i)->~T();
				}
			}
			m_Reserved += m_Size - count;
			m_Size = count;
		}
		if (m_Size < count)
		{
			Reserve(count);
			for (size_type i = m_Size; i < count; i++)
			{
				new (begin() + i) T{value};
			}
			m_Size = count;
		}
	}
	inline auto swap(Array& rhs) noexcept -> void
	{
		using atd::swap;
		atd::swap(m_Data, rhs.m_Data);
		atd::swap(m_Size, rhs.m_Size);
		atd::swap(m_Reserved, rhs.m_Reserved);
		atd::swap(m_MemoryOwner, rhs.m_MemoryOwner);
		atd::swap(m_ObjectOwner, rhs.m_ObjectOwner);
		atd::swap(m_UNUSED, rhs.m_UNUSED);
	}

	inline auto empty() const -> bool
	{
		return !m_Size;
	}

	inline auto data() const -> pointer
	{
		return m_Data;
	}

	inline auto front() -> reference
	{
		return *m_Data;
	}
	inline auto front() const -> const_reference
	{
		return *m_Data;
	}
	inline auto back() -> reference
	{
		return *(m_Data + m_Size - 1);
	}
	inline auto back() const -> const_reference
	{
		return *(m_Data + m_Size - 1);
	}

	inline auto begin() -> iterator
	{
		return m_Data;
	}
	inline auto begin() const -> const_iterator
	{
		return m_Data;
	}
	inline auto cbegin() const -> const_iterator
	{
		return m_Data;
	}
	inline auto end() -> iterator
	{
		return m_Data + Size();
	}
	inline auto end() const -> const_iterator
	{
		return m_Data + Size();
	}
	inline auto cend() const -> const_iterator
	{
		return m_Data + Size();
	}

	inline auto at(size_type index) -> reference
	{
		if (index > Size())
		{
			__debugbreak();
		}
		return *(begin() + index);
	}
	inline auto at(size_type index) const -> const_reference
	{
		if (index > Size())
		{
			__debugbreak();
		}
		return *(begin() + index);
	}

	inline auto operator[](size_type index) -> reference
	{
		return *(begin() + index);
	}
	inline auto operator[](size_type index) const -> const_reference
	{
		return *(begin() + index);
	}

	inline ~Array()
	{
		if (m_Data && m_MemoryOwner)
		{
			if (m_ObjectOwner)
			{
				for (size_type i = 0; i < Size(); i++)
				{
					(begin() + i)->~T();
				}
			}
			InternalFree(m_Data);
		}
		m_Data = nullptr;
	}

  private:
	inline auto InternalAlloc(size_type cnt) -> pointer
	{
		return reinterpret_cast<pointer>(new atd::uint8_t[cnt * sizeof(T)]);
	}
	inline auto InternalFree(pointer ptr) -> void
	{
		delete[] reinterpret_cast<atd::uint8_t*>(ptr);
	}
};
