#pragma once

#include "Common.h"

using ObjectID = atd::uint64_t;
using ObjectDescriptionID = atd::uint64_t;

template <typename T> static inline consteval auto ObjectIDForType()
{
	ObjectDescriptionID s{1};
	ObjectDescriptionID val{};
	for (atd::size_t i = 0; i < sizeof __FUNCSIG__; i++)
	{
		s += static_cast<ObjectDescriptionID>(__FUNCSIG__[i]);
	}
	for (auto i = 0; i < sizeof val; ++i)
	{
		s = 0x343FD * s + 0x269EC3;
		val |= (s & 0xFF) << (i * 8);
	}
	return val;
}

struct ObjectDescription
{
	const char* m_name{};
	ObjectDescriptionID m_id{};
	ObjectDescription* m_parent{};
	ObjectDescriptionID m_parentID{};
};

struct Object
{
	virtual ~Object() = default;
	virtual auto GetDescription() const -> ObjectDescription* const
	{
		return &g_StaticObjectDescription;
	}
	static inline ObjectDescription g_StaticObjectDescription{
		[]() -> ObjectDescription {
			return ObjectDescription{"Object", ObjectIDForType<Object>(),
									 nullptr, 0};
		}()};

	inline auto IsA(const ObjectDescription& desc) const -> bool
	{
		auto td = GetDescription();
		while (td)
		{
			if (td == &desc || td->m_parent == &desc)
			{
				return true;
			}
			td = td->m_parent;
		}
		return false;
	}

	template <typename T> inline auto IsA() const -> bool
	{
		// Uses ObjectIDForType insetad of g_StaticObjectDescription.m_id
		// because ObjectIDForType will work without the definiton of T
		const auto id = ObjectIDForType<T>();
		auto td = GetDescription();
		while (td)
		{
			if (td->m_id == id || td->m_parentID == id)
			{
				return true;
			}
			td = td->m_parent;
		}
		return false;
	}

	template <typename T> inline auto Cast() -> T*
	{
		if (IsA<T>())
		{
			return static_cast<T*>(this);
		}
		return nullptr;
	}

	template <typename T> inline auto Cast() const -> const T*
	{
		if (IsA<T>())
		{
			return static_cast<const T*>(this);
		}
		return nullptr;
	}
};

#define OBJECTBODYIMPL(objTypeName, objTypeNameParent)                         \
	static inline ObjectDescription g_StaticObjectDescription{                 \
		[]() -> ObjectDescription {                                            \
			return ObjectDescription{                                          \
				#objTypeName, ObjectIDForType<objTypeName>(),                  \
				&objTypeNameParent::g_StaticObjectDescription,                 \
				ObjectIDForType<objTypeNameParent>()};                         \
		}()};                                                                  \
	inline auto GetDescription() const->ObjectDescription* const override      \
	{                                                                          \
		return &g_StaticObjectDescription;                                     \
	}                                                                          \
	using Super = objTypeNameParent;
