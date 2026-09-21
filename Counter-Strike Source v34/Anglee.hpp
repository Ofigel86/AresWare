#pragma once

#include "Framework.hpp"

class Vector;

class QAngle0
{
public:
	QAngle0();
	QAngle0(float x, float y, float z);
	QAngle0(const QAngle0& angles);
	QAngle0(const float* angles);

	auto Init(float x = 0.f, float y = 0.f, float z = 0.f) -> void;

	auto ToArray() -> float*;
	auto ToArray() const -> const float*;

	auto At(unsigned int index) -> float&;
	auto At(unsigned int index) const -> const float;

	auto IsEqual(const QAngle0& angles) const -> bool;

	auto Add(const QAngle0& angles) -> void;
	auto Subtract(const QAngle0& angles) -> void;
	auto Multiply(const QAngle0& angles) -> void;
	auto Divide(const QAngle0& angles) -> void;

	auto Add(float value) -> void;
	auto Subtract(float value) -> void;
	auto Multiply(float value) -> void;
	auto Divide(float value) -> void;

	auto Negate() -> void;

	auto Dot(const QAngle0& angles) const -> float;

	auto LengthSquared() const -> float;
	auto Length() const -> float;

	auto DistanceSquared(const QAngle0& angles) const -> float;
	auto Distance(const QAngle0& angles) const -> float;

	auto Normalize() -> void;
	auto Clamp() -> void;

	auto ToVector(Vector* side = nullptr, Vector* up = nullptr) const->Vector;
	auto ToVectorTranspose(Vector* side = nullptr, Vector* up = nullptr) const->Vector;

public:
	auto operator = (const QAngle0& angles)->QAngle0&;

	auto operator [] (unsigned int index) -> float&;
	auto operator [] (unsigned int index) const -> const float;

	auto operator == (const QAngle0& angles) const -> bool;
	auto operator != (const QAngle0& angles) const -> bool;

public:
	auto operator += (const QAngle0& angles)->QAngle0&;
	auto operator -= (const QAngle0& angles)->QAngle0&;
	auto operator *= (const QAngle0& angles)->QAngle0&;
	auto operator /= (const QAngle0& angles)->QAngle0&;

	auto operator += (float value)->QAngle0&;
	auto operator -= (float value)->QAngle0&;
	auto operator *= (float value)->QAngle0&;
	auto operator /= (float value)->QAngle0&;

	auto operator + (const QAngle0& angles) const->QAngle0;
	auto operator - (const QAngle0& angles) const->QAngle0;
	auto operator * (const QAngle0& angles) const->QAngle0;
	auto operator / (const QAngle0& angles) const->QAngle0;

	auto operator + (float value) const->QAngle0;
	auto operator - (float value) const->QAngle0;
	auto operator * (float value) const->QAngle0;
	auto operator / (float value) const->QAngle0;

public:
	static QAngle0 Zero;

private:
	float m_x = 0.f;
	float m_y = 0.f;
	float m_z = 0.f;
};

inline auto operator * (float value, const QAngle0& angles) -> QAngle0
{
	return (value * angles);
}