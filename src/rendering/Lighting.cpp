#include "rendering/Lighting.hpp"

#include <cmath>

#include "rlgl.h"

namespace
{
  const char *kVertex = R"GLSL(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
uniform int uModelMode;   // 0: immediate-mode shapes (already in world space), 1: DrawModel
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
void main() {
  fragPosition = (uModelMode == 1) ? vec3(matModel * vec4(vertexPosition, 1.0)) : vertexPosition;
  fragNormal = (uModelMode == 1) ? mat3(matModel) * vertexNormal : vertexNormal;
  fragTexCoord = vertexTexCoord;
  fragColor = vertexColor;
  gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)GLSL";

  const char *kFragment = R"GLSL(#version 330
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform vec3 torchPos;
uniform vec3 torchColor;
uniform float torchPower;
uniform vec3 ambient;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float uEmissive;
out vec4 finalColor;
void main() {
  vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
  float nl = length(fragNormal);
  vec3 N = nl > 0.001 ? fragNormal / nl : vec3(0.0, 1.0, 0.0);
  vec3 L = torchPos - fragPosition;
  float d = length(L);
  L /= max(d, 0.001);
  float att = torchPower / (1.0 + 0.35 * d + 0.12 * d * d);
  float diff = 0.55 + 0.45 * max(dot(N, L), 0.0);
  vec3 col = base.rgb * (ambient + torchColor * att * diff);
  col = mix(col, base.rgb, uEmissive);
  float dist = length(viewPos - fragPosition);
  float fog = clamp(1.0 - exp(-fogDensity * dist * dist), 0.0, 1.0);
  col = mix(col, fogColor, fog * (1.0 - uEmissive));
  finalColor = vec4(col, base.a);
}
)GLSL";

  void setVec3(const Shader &s, int loc, Vector3 v) { SetShaderValue(s, loc, &v, SHADER_UNIFORM_VEC3); }
  void setFloat(const Shader &s, int loc, float v) { SetShaderValue(s, loc, &v, SHADER_UNIFORM_FLOAT); }
  void setInt(const Shader &s, int loc, int v) { SetShaderValue(s, loc, &v, SHADER_UNIFORM_INT); }
} // namespace

Lighting::Lighting()
{
  shader_ = LoadShaderFromMemory(kVertex, kFragment);
  locView_ = GetShaderLocation(shader_, "viewPos");
  locTorch_ = GetShaderLocation(shader_, "torchPos");
  locColor_ = GetShaderLocation(shader_, "torchColor");
  locPower_ = GetShaderLocation(shader_, "torchPower");
  locAmbient_ = GetShaderLocation(shader_, "ambient");
  locFog_ = GetShaderLocation(shader_, "fogColor");
  locFogDensity_ = GetShaderLocation(shader_, "fogDensity");
  locModelMode_ = GetShaderLocation(shader_, "uModelMode");
  locEmissive_ = GetShaderLocation(shader_, "uEmissive");
}

Lighting::~Lighting() { UnloadShader(shader_); }

void Lighting::update(Vector3 cameraPos, Vector3 torchPos, bool torchOn, float t)
{
  const float flicker = 1.f + 0.07f * std::sin(t * 13.f) + 0.05f * std::sin(t * 27.f + 1.f) + 0.03f * std::sin(t * 51.f);
  setVec3(shader_, locView_, cameraPos);
  setVec3(shader_, locTorch_, torchPos);
  setVec3(shader_, locColor_, {1.0f, 0.62f, 0.30f});
  setFloat(shader_, locPower_, torchOn ? 6.5f * flicker : 0.f);
  setVec3(shader_, locAmbient_, {0.06f, 0.06f, 0.09f});
  setVec3(shader_, locFog_, {0.015f, 0.015f, 0.025f});
  setFloat(shader_, locFogDensity_, 0.0025f);
  setInt(shader_, locModelMode_, 0);
  setFloat(shader_, locEmissive_, 0.f);
}

void Lighting::setModelMode(bool on)
{
  rlDrawRenderBatchActive(); // flush pending shapes before the uniform changes
  setInt(shader_, locModelMode_, on ? 1 : 0);
}

void Lighting::setEmissive(float v)
{
  rlDrawRenderBatchActive();
  setFloat(shader_, locEmissive_, v);
}
