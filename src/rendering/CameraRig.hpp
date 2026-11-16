#pragma once
#include <raylib.h>

#include "world/Dungeon.hpp"

// Third-person orbit camera with wall collision, plus an optional first-person mode.
class CameraRig
{
public:
  CameraRig();
  void look(Vector2 mouseDelta);
  void update(Vector3 playerPos, const Dungeon &dungeon);
  void toggleFirstPerson() { firstPerson_ = !firstPerson_; }
  bool firstPerson() const { return firstPerson_; }
  float yaw() const { return yaw_; }
  const Camera3D &camera() const { return cam_; }

private:
  Camera3D cam_{};
  float yaw_ = 0.f, pitch_ = 0.45f, distance_ = 6.f;
  bool firstPerson_ = false;
};
