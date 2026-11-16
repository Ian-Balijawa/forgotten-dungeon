#pragma once
#include <raylib.h>

#include "rendering/CameraRig.hpp"
#include "rendering/Lighting.hpp"
#include "world/World.hpp"

class Renderer
{
public:
  explicit Renderer(Lighting &lighting);
  ~Renderer();
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  void drawWorld(const World &world, const CameraRig &camera, float time);

private:
  void drawDungeon(const World &w, Vector3 camPos);
  void drawSkeleton(const Enemy &e, float time);
  void drawPlayer(const Player &p, float time);
  void drawPickup(const Pickup &pk, float time);

  Lighting &lighting_;
  Model skeletonModel_{};
  bool hasSkeletonModel_ = false;
};
