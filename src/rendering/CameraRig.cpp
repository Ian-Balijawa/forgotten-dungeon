#include "rendering/CameraRig.hpp"

#include <algorithm>
#include <cmath>

CameraRig::CameraRig()
{
  cam_.up = {0.f, 1.f, 0.f};
  cam_.fovy = 70.f;
  cam_.projection = CAMERA_PERSPECTIVE;
  cam_.position = {0.f, 3.f, -6.f};
  cam_.target = {0.f, 1.f, 0.f};
}

void CameraRig::look(Vector2 d)
{
  yaw_ -= d.x * 0.003f;
  pitch_ = std::clamp(pitch_ + d.y * 0.003f, 0.05f, 1.0f);
}

void CameraRig::update(Vector3 p, const Dungeon &dungeon)
{
  if (firstPerson_)
  {
    const Vector3 eye{p.x, p.y + 1.6f, p.z};
    const float el = -(pitch_ - 0.45f) * 1.2f;
    cam_.position = eye;
    cam_.target = {eye.x + std::sin(yaw_) * std::cos(el), eye.y + std::sin(el), eye.z + std::cos(yaw_) * std::cos(el)};
    return;
  }
  const Vector3 head{p.x, p.y + 1.4f, p.z};
  const float cp = std::cos(pitch_);
  const Vector3 offset{-std::sin(yaw_) * cp * distance_, std::sin(pitch_) * distance_, -std::cos(yaw_) * cp * distance_};

  // Camera collision: march from the head toward the desired position and stop before any wall.
  float safe = 1.f;
  constexpr int kSteps = 24;
  for (int i = 1; i <= kSteps; ++i)
  {
    const float f = static_cast<float>(i) / kSteps;
    const float x = head.x + offset.x * f, y = head.y + offset.y * f, z = head.z + offset.z * f;
    if (dungeon.solidAt(x, z) || y > cfg::kWallH - 0.25f || y < 0.25f)
    {
      safe = std::max(0.f, static_cast<float>(i - 1) / kSteps - 0.04f);
      break;
    }
  }
  cam_.position = {head.x + offset.x * safe, head.y + offset.y * safe, head.z + offset.z * safe};
  cam_.target = head;
}
