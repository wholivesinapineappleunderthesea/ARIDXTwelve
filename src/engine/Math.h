#pragma once

#include <limits>

namespace Math
{
	static inline constexpr auto Pi = 3.14159265358979323846f;
	static inline constexpr auto TwoPi = 6.28318530717958647693f;
	static inline constexpr auto NaN = std::numeric_limits<float>::quiet_NaN();
	static inline constexpr auto FDegreesToRadians(float degrees) -> float
	{
		return degrees * Pi / 180.f;
	}
	static inline constexpr auto FRadiansToDegrees(float radians) -> float
	{
		return radians * 180.f / Pi;
	}
	static inline constexpr auto FMin(float a, float b) -> float
	{
		return a < b ? a : b;
	}
	static inline constexpr auto FMax(float a, float b) -> float
	{
		return a > b ? a : b;
	}
	static inline constexpr auto FAbs(float x) -> float
	{
		if (x == -0.f)
			return 0.f;
		return x < 0.f ? -x : x;
	}
	static inline constexpr auto FMod(float x, float y) -> float
	{
		const auto q = x / y;
		const auto w = static_cast<float>(static_cast<int>(q));
		const auto r = x - w * y;
		return r + (r < 0.f) * y;
	}
	static inline constexpr auto FSqrt(float x) -> float
	{
		if (x == 0.f)
			return 0.f;
		if (x < 0.f)
			return NaN;

		auto s = x;
		auto y = x * 0.5f;
		while (y < s)
		{
			s = y;
			y = (y + x / y) * 0.5f;
		}

		return s;
	}
	static inline constexpr auto FSin(float x) -> float
	{
		/*
		Taylor series:
		sin x = x - (x^3)/(3!) + (x^5)/(5!) - (x^7)/(7!) + (x^9)/(9!)
		*/
		using namespace Math;

		const auto a = FMod(x + Pi, TwoPi) - Pi;

		const auto a3 = a * a * a;
		const auto a5 = a3 * a * a;
		const auto a7 = a5 * a * a;
		const auto a9 = a7 * a * a;

		return a - (a3 / 6.f) + (a5 / 120.f) - (a7 / 5040.f) + (a9 / 362880.f);
	}
	static inline constexpr auto FCos(float x) -> float
	{
		/*
		Taylor series:
		cos x = 1 - (x^2)/(2!) + (x^4)/(4!) - (x^6)/(6!) + (x^8)/(8!)
		*/
		using namespace Math;

		const auto a = FMod(x + Pi, TwoPi) - Pi;

		const auto a2 = a * a;
		const auto a4 = a2 * a2;
		const auto a6 = a4 * a2;
		const auto a8 = a6 * a2;

		return 1.f - (a2 / 2.f) + (a4 / 24.f) - (a6 / 720.f) + (a8 / 40320.f);
	}
	static inline constexpr auto FTan(float x) -> float
	{
		/*
		Taylor series:
		tan x = x + (x^3)/3 + (2x^5)/15 + (17x^7)/315 + (62x^9)/2835
		*/
		using namespace Math;

		const auto a = FMod(x + Pi, TwoPi) - Pi;

		const auto a3 = a * a * a;
		const auto a5 = a3 * a * a;
		const auto a7 = a5 * a * a;
		const auto a9 = a7 * a * a;

		return a + (a3 / 3.f) + (2.f * a5 / 15.f) + (17.f * a7 / 315.f) +
			   (62.f * a9 / 2835.f);
	}
	static inline constexpr auto FAsin(float x) -> float
	{
		/*
		Taylor series:
		asin x = x + (x^3)/6 + (3x^5)/40 + (5x^7)/112 + (35x^9)/1152
		*/
		using namespace Math;

		const auto a = x;
		const auto a3 = a * a * a;
		const auto a5 = a3 * a * a;
		const auto a7 = a5 * a * a;
		const auto a9 = a7 * a * a;

		return a + (a3 / 6.f) + (3.f * a5 / 40.f) + (5.f * a7 / 112.f) +
			   (35.f * a9 / 1152.f);
	}
	static inline constexpr auto FAtan2(float y, float x) -> float
	{
		/*
		Taylor series:
		atan x = x - (x^3)/3 + (x^5)/5 - (x^7)/7 + (x^9)/9
		*/
		using namespace Math;

		if (x == 0.f)
		{
			if (y == 0.f)
				return 0.f;
			return y > 0.f ? Pi / 2.f : -Pi / 2.f;
		}

		const auto a = y / x;
		const auto a2 = a * a;
		const auto a3 = a2 * a;
		const auto a5 = a3 * a * a;
		const auto a7 = a5 * a * a;
		const auto a9 = a7 * a * a;

		const auto r = a - (a3 / 3.f) + (a5 / 5.f) - (a7 / 7.f) + (a9 / 9.f);

		return x > 0.f ? r : r + Pi;
	}

	struct Vec2;
	struct Vec3;
	struct Vec4;
	union Mat4x4;
	struct Quat;

	struct Vec2
	{
		inline Vec2() = default;
		inline Vec2(float x, float y) : x(x), y(y)
		{
		}
		inline auto operator+=(const Vec2& rhs) -> Vec2&
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		inline auto operator-=(const Vec2& rhs) -> Vec2&
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		inline auto operator*=(const Vec2& rhs) -> Vec2&
		{
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}
		inline auto operator/=(const Vec2& rhs) -> Vec2&
		{
			x /= rhs.x;
			y /= rhs.y;
			return *this;
		}
		inline auto operator*=(float rhs)
		{
			x *= rhs;
			y *= rhs;
			return *this;
		}
		inline auto operator/=(float rhs)
		{
			x /= rhs;
			y /= rhs;
			return *this;
		}
		inline auto Length() const -> float
		{
			return FSqrt(x * x + y * y);
		}
		inline auto LengthSquared() const -> float
		{
			return x * x + y * y;
		}
		inline auto Normalized() const -> Vec2
		{
			const auto l = Length();
			return Vec2{x / l, y / l};
		}
		inline auto Normalize() -> void
		{
			*this /= Length();
		}

		inline auto Dot(const Vec2& rhs) const -> float
		{
			return x * rhs.x + y * rhs.y;
		}
		inline auto Cross(const Vec2& rhs) const -> float
		{
			return x * rhs.y - y * rhs.x;
		}

		inline auto operator-() const -> Vec2
		{
			return Vec2{-x, -y};
		}

		float x{}, y{};
	};
	inline auto operator+(const Vec2& lhs, const Vec2& rhs) -> Vec2
	{
		return Vec2{lhs.x + rhs.x, lhs.y + rhs.y};
	}
	inline auto operator-(const Vec2& lhs, const Vec2& rhs) -> Vec2
	{
		return Vec2{lhs.x - rhs.x, lhs.y - rhs.y};
	}
	inline auto operator*(const Vec2& lhs, const Vec2& rhs) -> Vec2
	{
		return Vec2{lhs.x * rhs.x, lhs.y * rhs.y};
	}
	inline auto operator/(const Vec2& lhs, const Vec2& rhs) -> Vec2
	{
		return Vec2{lhs.x / rhs.x, lhs.y / rhs.y};
	}
	inline auto operator*(const Vec2& lhs, float rhs) -> Vec2
	{
		return Vec2{lhs.x * rhs, lhs.y * rhs};
	}
	inline auto operator/(const Vec2& lhs, float rhs) -> Vec2
	{
		return Vec2{lhs.x / rhs, lhs.y / rhs};
	}
	inline auto operator*(float lhs, const Vec2& rhs) -> Vec2
	{
		return Vec2{lhs * rhs.x, lhs * rhs.y};
	}
	inline auto operator/(float lhs, const Vec2& rhs) -> Vec2
	{
		return Vec2{lhs / rhs.x, lhs / rhs.y};
	}
	inline auto operator==(const Vec2& lhs, const Vec2& rhs) -> bool
	{
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}
	inline auto operator!=(const Vec2& lhs, const Vec2& rhs) -> bool
	{
		return lhs.x != rhs.x || lhs.y != rhs.y;
	}

	struct Vec3
	{
		inline Vec3() = default;
		inline Vec3(const Vec2& v, float z = 0.f) : x(v.x), y(v.y), z(z)
		{
		}
		inline Vec3(float x, float y, float z) : x(x), y(y), z(z)
		{
		}
		inline auto operator+=(const Vec3& rhs) -> Vec3&
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}
		inline auto operator-=(const Vec3& rhs) -> Vec3&
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}
		inline auto operator*=(const Vec3& rhs) -> Vec3&
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			return *this;
		}
		inline auto operator/=(const Vec3& rhs) -> Vec3&
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			return *this;
		}
		inline auto operator*=(float rhs) -> Vec3&
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
			return *this;
		}
		inline auto operator/=(float rhs) -> Vec3&
		{
			x /= rhs;
			y /= rhs;
			z /= rhs;
			return *this;
		}
		inline auto Length() const -> float
		{
			return FSqrt(x * x + y * y + z * z);
		}
		inline auto LengthSquared() const -> float
		{
			return x * x + y * y + z * z;
		}
		inline auto Normalized() const -> Vec3
		{
			const auto l = Length();
			return Vec3{x / l, y / l, z / l};
		}
		inline auto Normalize() -> void
		{
			*this /= Length();
		}

		inline auto Dot(const Vec3& rhs) const -> float
		{
			return x * rhs.x + y * rhs.y + z * rhs.z;
		}
		inline auto Cross(const Vec3& rhs) const -> Vec3
		{
			return Vec3{y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z,
						x * rhs.y - y * rhs.x};
		}

		inline auto operator-() const -> Vec3
		{
			return Vec3{-x, -y, -z};
		}

		inline operator Vec2() const
		{
			return Vec2{x, y};
		}

		// Constant vectors:
		static inline auto Up() -> Vec3
		{
			return Vec3{0.f, 1.f, 0.f};
		}
		static inline auto Right() -> Vec3
		{
			return Vec3{1.f, 0.f, 0.f};
		}
		static inline auto Forward() -> Vec3
		{
			return Vec3{0.f, 0.f, 1.f};
		}

		float x{}, y{}, z{};
	};
	inline auto operator+(const Vec3& lhs, const Vec3& rhs) -> Vec3
	{
		return Vec3{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
	}
	inline auto operator-(const Vec3& lhs, const Vec3& rhs) -> Vec3
	{
		return Vec3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
	}
	inline auto operator*(const Vec3& lhs, const Vec3& rhs) -> Vec3
	{
		return Vec3{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
	}
	inline auto operator/(const Vec3& lhs, const Vec3& rhs) -> Vec3
	{
		return Vec3{lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
	}
	inline auto operator*(const Vec3& lhs, float rhs) -> Vec3
	{
		return Vec3{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs};
	}
	inline auto operator/(const Vec3& lhs, float rhs) -> Vec3
	{
		return Vec3{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs};
	}
	inline auto operator*(float lhs, const Vec3& rhs) -> Vec3
	{
		return Vec3{lhs * rhs.x, lhs * rhs.y, lhs * rhs.z};
	}
	inline auto operator/(float lhs, const Vec3& rhs) -> Vec3
	{
		return Vec3{lhs / rhs.x, lhs / rhs.y, lhs / rhs.z};
	}
	inline auto operator==(const Vec3& lhs, const Vec3& rhs) -> bool
	{
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
	}
	inline auto operator!=(const Vec3& lhs, const Vec3& rhs) -> bool
	{
		return !(lhs == rhs);
	}

	struct Vec4
	{
		inline Vec4() = default;
		inline Vec4(const Vec2& v, float z = 0.f, float w = 0.f)
			: x(v.x), y(v.y), z(z), w(w)
		{
		}
		inline Vec4(const Vec3& v, float w = 0.f) : x(v.x), y(v.y), z(v.z), w(w)
		{
		}
		inline Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
		{
		}
		inline auto operator+=(const Vec4& rhs) -> Vec4&
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			w += rhs.w;
			return *this;
		}
		inline auto operator-=(const Vec4& rhs) -> Vec4&
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			w -= rhs.w;
			return *this;
		}
		inline auto operator*=(const Vec4& rhs) -> Vec4&
		{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			w *= rhs.w;
			return *this;
		}
		inline auto operator/=(const Vec4& rhs) -> Vec4&
		{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			w /= rhs.w;
			return *this;
		}
		inline auto operator*=(float rhs) -> Vec4&
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
			w *= rhs;
			return *this;
		}
		inline auto operator/=(float rhs) -> Vec4&
		{
			x /= rhs;
			y /= rhs;
			z /= rhs;
			w /= rhs;
			return *this;
		}
		inline auto Length() const -> float
		{
			return FSqrt(x * x + y * y + z * z + w * w);
		}
		inline auto LengthSquared() const -> float
		{
			return x * x + y * y + z * z + w * w;
		}
		inline auto Normalized() const -> Vec4
		{
			const auto l = Length();
			return Vec4{x / l, y / l, z / l, w / l};
		}
		inline auto Normalize() -> void
		{
			*this /= Length();
		}

		inline auto Dot(const Vec4& rhs) const -> float
		{
			return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
		}
		inline auto Cross(const Vec4& rhs) const -> Vec4
		{
			return Vec4{y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z,
						x * rhs.y - y * rhs.x, 0.f};
		}

		inline auto operator-() const -> Vec4
		{
			return Vec4{-x, -y, -z, -w};
		}

		inline operator Vec2() const
		{
			return Vec2{x, y};
		}
		inline operator Vec3() const
		{
			return Vec3{x, y, z};
		}

		float x{}, y{}, z{}, w{};
	};
	inline auto operator+(const Vec4& lhs, const Vec4& rhs) -> Vec4
	{
		return Vec4{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
	}
	inline auto operator-(const Vec4& lhs, const Vec4& rhs) -> Vec4
	{
		return Vec4{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
	}
	inline auto operator*(const Vec4& lhs, const Vec4& rhs) -> Vec4
	{
		return Vec4{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
	}
	inline auto operator/(const Vec4& lhs, const Vec4& rhs) -> Vec4
	{
		return Vec4{lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
	}
	inline auto operator*(const Vec4& lhs, float rhs) -> Vec4
	{
		return Vec4{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs};
	}
	inline auto operator/(const Vec4& lhs, float rhs) -> Vec4
	{
		return Vec4{lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs};
	}
	inline auto operator*(float lhs, const Vec4& rhs) -> Vec4
	{
		return Vec4{lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w};
	}
	inline auto operator/(float lhs, const Vec4& rhs) -> Vec4
	{
		return Vec4{lhs / rhs.x, lhs / rhs.y, lhs / rhs.z, lhs / rhs.w};
	}
	inline auto operator==(const Vec4& lhs, const Vec4& rhs) -> bool
	{
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z &&
			   lhs.w == rhs.w;
	}
	inline auto operator!=(const Vec4& lhs, const Vec4& rhs) -> bool
	{
		return !(lhs == rhs);
	}

	union Mat4x4
	{
		inline Mat4x4(float v = 1.f)
		{
			rows[0] = Vec4{v, 0.f, 0.f, 0.f};
			rows[1] = Vec4{0.f, v, 0.f, 0.f};
			rows[2] = Vec4{0.f, 0.f, v, 0.f};
			rows[3] = Vec4{0.f, 0.f, 0.f, v};
		}
		inline Mat4x4(const Vec4& a, const Vec4& b, const Vec4& c,
					  const Vec4& d)
		{
			rows[0] = a;
			rows[1] = b;
			rows[2] = c;
			rows[3] = d;
		}
		inline Mat4x4(const Quat& rot);
		inline auto operator[](int index) -> float*
		{
			return m[index];
		}
		inline auto operator[](int index) const -> const float*
		{
			return m[index];
		}
		inline auto operator*=(const Mat4x4& rhs) -> Mat4x4&
		{
			Mat4x4 result{};
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
					result[i][j] =
						rows[i].x * rhs[0][j] + rows[i].y * rhs[1][j] +
						rows[i].z * rhs[2][j] + rows[i].w * rhs[3][j];
			}
			*this = result;
			return *this;
		}
		inline auto Transposed() const -> Mat4x4
		{
			Mat4x4 result{};
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
					result[i][j] = m[j][i];
			}
			return result;
		}

		static inline auto Identity() -> Mat4x4
		{
			return Mat4x4{1.f};
		}

		static inline auto PerspectiveRH_ZO(float fovy, float aspect,
											float znear, float zfar) -> Mat4x4
		{
			const auto tanHalfFovy = FTan(fovy / 2.f);
			Mat4x4 result{0.f};
			result[0][0] = 1.f / (aspect * tanHalfFovy);
			result[1][1] = 1.f / tanHalfFovy;
			result[2][2] = zfar / (znear - zfar);
			result[2][3] = -1.f;
			result[3][2] = -(zfar * znear) / (zfar - znear);
			return result;
		}

		static inline auto LookAtRH(const Vec3& eye, const Vec3& center,
									const Vec3& up) -> Mat4x4
		{
			const auto f = (center - eye).Normalized();
			const auto s = f.Cross(up).Normalized();
			const auto u = s.Cross(f);
			Mat4x4 result{1.f};
			result[0][0] = s.x;
			result[1][0] = s.y;
			result[2][0] = s.z;
			result[0][1] = u.x;
			result[1][1] = u.y;
			result[2][1] = u.z;
			result[0][2] = -f.x;
			result[1][2] = -f.y;
			result[2][2] = -f.z;
			result[3][0] = -s.Dot(eye);
			result[3][1] = -u.Dot(eye);
			result[3][2] = f.Dot(eye);
			return result;
		}

		static inline auto ToQuat(const Mat4x4& mat) -> Quat;

		float m[4][4];
		Vec4 rows[4];
	};

	inline auto operator*(const Mat4x4& lhs, const Mat4x4& rhs) -> Mat4x4
	{
		Mat4x4 result{};
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
				result[i][j] =
					lhs.rows[i].x * rhs[0][j] + lhs.rows[i].y * rhs[1][j] +
					lhs.rows[i].z * rhs[2][j] + lhs.rows[i].w * rhs[3][j];
		}
		return result;
	}
	inline auto operator*(const Mat4x4& lhs, const Vec4& rhs) -> Vec4
	{
		Vec4 result{};
		result.x = lhs.rows[0].x * rhs.x + lhs.rows[0].y * rhs.y +
				   lhs.rows[0].z * rhs.z + lhs.rows[0].w * rhs.w;
		result.y = lhs.rows[1].x * rhs.x + lhs.rows[1].y * rhs.y +
				   lhs.rows[1].z * rhs.z + lhs.rows[1].w * rhs.w;
		result.z = lhs.rows[2].x * rhs.x + lhs.rows[2].y * rhs.y +
				   lhs.rows[2].z * rhs.z + lhs.rows[2].w * rhs.w;
		return result;
	}

	inline auto Translate(const Mat4x4& m, const Vec3& v) -> Mat4x4
	{
		Mat4x4 result{m};
		result.rows[3] =
			m.rows[0] * v.x + m.rows[1] * v.y + m.rows[2] * v.z + m.rows[3];
		return result;
	}

	inline auto Scale(const Mat4x4& m, const Vec3& v) -> Mat4x4
	{
		Mat4x4 result{};
		result.rows[0] = m.rows[0] * v.x;
		result.rows[1] = m.rows[1] * v.y;
		result.rows[2] = m.rows[2] * v.z;
		result.rows[3] = m.rows[3];
		return result;
	}

	struct Quat
	{
		inline Quat() = default;
		inline Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
		{
		}
		inline Quat(const Vec3& axis, float angle)
		{
			const auto halfAngle = angle / 2.f;
			const auto s = Math::FSin(halfAngle);
			x = axis.x * s;
			y = axis.y * s;
			z = axis.z * s;
			w = Math::FCos(halfAngle);
		}
		inline Quat(const Mat4x4& rot)
		{
			*this = Mat4x4::ToQuat(rot);
		}

		inline auto operator*(const Quat& rhs) const -> Quat
		{
			return Quat{w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
						w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z,
						w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x,
						w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z};
		}
		inline auto operator/(const Quat& rhs) const -> Quat
		{
			return *this * rhs.Inverse();
		}
		inline auto operator-() const -> Quat
		{
			return Quat{-x, -y, -z, w};
		}

		inline auto Pitch() -> float
		{
			return FAtan2(2.f * (w * x + y * z), 1.f - 2.f * (x * x + y * y));
		}
		inline auto Yaw() -> float
		{
			return FAsin(2.f * (w * y - z * x));
		}
		inline auto Roll() -> float
		{
			return FAtan2(2.f * (w * z + x * y), 1.f - 2.f * (y * y + z * z));
		}
		inline auto Length() const -> float
		{
			return Math::FSqrt(x * x + y * y + z * z + w * w);
		}
		inline auto LengthSquared() const -> float
		{
			return x * x + y * y + z * z + w * w;
		}
		inline auto Inverse() const -> Quat
		{
			const auto len = LengthSquared();
			return Quat{-x / len, -y / len, -z / len, w / len};
		}

		inline auto ToMat4x4() const -> Mat4x4
		{
			const auto x2 = x * x;
			const auto y2 = y * y;
			const auto z2 = z * z;
			const auto xy = x * y;
			const auto xz = x * z;
			const auto yz = y * z;
			const auto wx = w * x;
			const auto wy = w * y;
			const auto wz = w * z;
			Mat4x4 result{};
			result[0][0] = 1.f - 2.f * (y2 + z2);
			result[0][1] = 2.f * (xy - wz);
			result[0][2] = 2.f * (xz + wy);
			result[1][0] = 2.f * (xy + wz);
			result[1][1] = 1.f - 2.f * (x2 + z2);
			result[1][2] = 2.f * (yz - wx);
			result[2][0] = 2.f * (xz - wy);
			result[2][1] = 2.f * (yz + wx);
			result[2][2] = 1.f - 2.f * (x2 + y2);
			result[3][3] = 1.f;
			return result;
		}

		float x{1.f}, y{}, z{}, w{};
	};
	Mat4x4::Mat4x4(const Quat& rot)
	{
		*this = rot.ToMat4x4();
	}
	auto Mat4x4::ToQuat(const Mat4x4& mat) -> Quat
	{
		const auto trace = mat[0][0] + mat[1][1] + mat[2][2];
		if (trace > 0.f)
		{
			const auto s = 0.5f / Math::FSqrt(trace + 1.f);
			return Quat{(mat[2][1] - mat[1][2]) * s,
						(mat[0][2] - mat[2][0]) * s,
						(mat[1][0] - mat[0][1]) * s, 0.25f / s};
		}
		else if (mat[0][0] > mat[1][1] && mat[0][0] > mat[2][2])
		{
			const auto s =
				2.f * Math::FSqrt(1.f + mat[0][0] - mat[1][1] - mat[2][2]);
			return Quat{0.25f * s, (mat[0][1] + mat[1][0]) / s,
						(mat[0][2] + mat[2][0]) / s,
						(mat[2][1] - mat[1][2]) / s};
		}
		else if (mat[1][1] > mat[2][2])
		{
			const auto s =
				2.f * Math::FSqrt(1.f + mat[1][1] - mat[0][0] - mat[2][2]);
			return Quat{(mat[0][1] + mat[1][0]) / s, 0.25f * s,
						(mat[1][2] + mat[2][1]) / s,
						(mat[0][2] - mat[2][0]) / s};
		}
		else
		{
			const auto s =
				2.f * Math::FSqrt(1.f + mat[2][2] - mat[0][0] - mat[1][1]);
			return Quat{(mat[0][2] + mat[2][0]) / s,
						(mat[1][2] + mat[2][1]) / s, 0.25f * s,
						(mat[1][0] - mat[0][1]) / s};
		}
	}

	inline auto Rotate(const Mat4x4& m, const Quat& q) -> Mat4x4
	{
		return m * q.ToMat4x4();
	}

	inline auto RotatePoint(const Vec3& v, const Quat& q) -> Vec3
	{
		const auto qv = Vec3{q.x, q.y, q.z};
		const auto uv = qv.Cross(v);
		const auto uuv = qv.Cross(uv);
		return v + ((uv * q.w) + uuv) * 2.f;
	}

	// Physics helpers:
	static inline auto DoesLineSegmentIntersectWithAABB(const Vec3& begin,
														const Vec3& end,
														const Vec3& min,
														const Vec3& max) -> bool
	{
		if (begin.x < min.x && end.x < min.x)
			return false;
		if (begin.x > max.x && end.x > max.x)
			return false;
		if (begin.y < min.y && end.y < min.y)
			return false;
		if (begin.y > max.y && end.y > max.y)
			return false;
		if (begin.z < min.z && end.z < min.z)
			return false;
		if (begin.z > max.z && end.z > max.z)
			return false;
		return true;
	}

	static inline auto FindLineSegmentIntersectionWithAABB(
		const Vec3& begin, const Vec3& end, const Vec3& min, const Vec3& max,
		Vec3& positionHit, float& distanceHit, Vec3& normalHit) -> bool
	{
		// Find candidate plane intersections:
		const auto tx1 = (min.x - begin.x) / (end.x - begin.x);
		const auto tx2 = (max.x - begin.x) / (end.x - begin.x);
		const auto ty1 = (min.y - begin.y) / (end.y - begin.y);
		const auto ty2 = (max.y - begin.y) / (end.y - begin.y);
		const auto tz1 = (min.z - begin.z) / (end.z - begin.z);
		const auto tz2 = (max.z - begin.z) / (end.z - begin.z);
		// Find candidate plane intersection distances:
		const auto tmin =
			Math::FMax(Math::FMax(Math::FMin(tx1, tx2), Math::FMin(ty1, ty2)),
					   Math::FMin(tz1, tz2));
		const auto tmax =
			Math::FMin(Math::FMin(Math::FMax(tx1, tx2), Math::FMax(ty1, ty2)),
					   Math::FMax(tz1, tz2));
		// If candidate plane intersection distances are outside the line
		// segment, no intersection:
		if (tmax < 0.f || tmin > tmax)
			return false;
		// If the intersection point is outside the line segment, no
		// intersection:
		if (tmin < 0.f)
			distanceHit = tmax;
		else
			distanceHit = tmin;
		// If the intersection point is outside the line segment, no
		// intersection:
		if (distanceHit < 0.f || distanceHit > 1.f)
			return false;
		// Calculate the intersection point:
		positionHit = begin + (end - begin) * distanceHit;
		// Calculate the intersection normal:
		if (distanceHit == tx1)
			normalHit = Vec3{-1.f, 0.f, 0.f};
		else if (distanceHit == tx2)
			normalHit = Vec3{1.f, 0.f, 0.f};
		else if (distanceHit == ty1)
			normalHit = Vec3{0.f, -1.f, 0.f};
		else if (distanceHit == ty2)
			normalHit = Vec3{0.f, 1.f, 0.f};
		else if (distanceHit == tz1)
			normalHit = Vec3{0.f, 0.f, -1.f};
		else if (distanceHit == tz2)
			normalHit = Vec3{0.f, 0.f, 1.f};

		return true;
	}

}; // namespace Math
