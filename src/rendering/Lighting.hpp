#pragma once
#include <raylib.h>

// Per-pixel torch light (point light with flicker + falloff) and distance fog.
class Lighting {
 public:
  Lighting();
  ~Lighting();
  Lighting(const Lighting&) = delete;
  Lighting& operator=(const Lighting&) = delete;

  void update(Vector3 cameraPos, Vector3 torchPos, bool torchOn, float time);
  void setModelMode(bool on);    // true when drawing loaded models (uses matModel)
  void setEmissive(float value); // 0 = lit, 1 = self-illuminated (ghosts, artifact)
  const Shader& shader() const { return shader_; }

 private:
  Shader shader_{};
  int locView_ = -1, locTorch_ = -1, locColor_ = -1, locPower_ = -1, locAmbient_ = -1;
  int locFog_ = -1, locFogDensity_ = -1, locModelMode_ = -1, locEmissive_ = -1;
};
