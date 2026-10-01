#pragma once
#include <algorithm>
#include <cmath>

namespace physics {
inline constexpr float kGravity = 22.f;

// Circle (player/enemy footprint) vs axis-aligned square (pushable block), on the XZ plane.
inline bool circleVsSquare(float cx, float cz, float r, float bx, float bz, float half) {
  const float px = std::clamp(cx, bx - half, bx + half);
  const float pz = std::clamp(cz, bz - half, bz + half);
  const float dx = cx - px, dz = cz - pz;
  return dx * dx + dz * dz < r * r;
}
}  // namespace physics
