#include "rendering/Renderer.hpp"

#include <cmath>

#include "core/Config.hpp"
#include "core/Paths.hpp"

namespace {
constexpr float kRad2Deg = 57.29578f;
Vector3 add(Vector3 a, Vector3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vector3 up(Vector3 a, float y) { return {a.x, a.y + y, a.z}; }
Color wallColor(int x, int z) {
  const unsigned h = static_cast<unsigned>(x * 73856093u ^ z * 19349663u) % 24u;
  return Color{static_cast<unsigned char>(88 + h), static_cast<unsigned char>(84 + h), static_cast<unsigned char>(92 + h), 255};
}
}  // namespace

Renderer::Renderer(Lighting& lighting) : lighting_(lighting) {
  // Optional: use a real 3D model for skeletons if the file exists (glTF/OBJ/IQM supported by raylib).
  const std::string path = assetPath("models/skeleton.glb");
  if (!path.empty()) {
    skeletonModel_ = LoadModel(path.c_str());
    for (int i = 0; i < skeletonModel_.materialCount; ++i) skeletonModel_.materials[i].shader = lighting_.shader();
    hasSkeletonModel_ = skeletonModel_.meshCount > 0;
  }
}

Renderer::~Renderer() {
  if (hasSkeletonModel_) UnloadModel(skeletonModel_);
}

void Renderer::drawWorld(const World& w, const CameraRig& rig, float time) {
  const Camera3D& cam = rig.camera();
  const Player& p = w.player;
  const Vector3 torch{p.pos.x + std::sin(p.facing) * 0.5f, p.pos.y + 1.7f, p.pos.z + std::cos(p.facing) * 0.5f};
  lighting_.update(cam.position, torch, p.torchOn, time);

  BeginMode3D(cam);
  BeginShaderMode(lighting_.shader());

  drawDungeon(w, cam.position);

  if (w.hasPlate) {
    const Color plate = w.plateDown ? Color{90, 200, 110, 255} : Color{200, 170, 70, 255};
    DrawCube({w.platePos.x, 0.03f, w.platePos.z}, 1.7f, 0.06f, 1.7f, plate);
    const float h = w.block.half;
    DrawCube(w.block.pos, h * 2, h * 2, h * 2, Color{118, 112, 120, 255});
    DrawCubeWires(w.block.pos, h * 2.01f, h * 2.01f, h * 2.01f, Color{50, 48, 54, 255});
  }
  for (const auto& pk : w.pickups) if (!pk.taken) drawPickup(pk, time);
  for (const auto& e : w.enemies) {
    if (e.state == EnemyState::Dead && e.deathTimer > 8.f) continue;
    drawSkeleton(e, time);
  }
  if (!rig.firstPerson()) drawPlayer(p, time);

  if (w.echoActive()) {  // translucent ghost replaying the past
    lighting_.setEmissive(1.f);
    const Vector3 g = w.ghostPos();
    const Color gc{130, 190, 255, 120};
    DrawCylinderEx(up(g, 0.1f), up(g, 1.5f), 0.3f, 0.25f, 10, gc);
    DrawSphere(up(g, 1.75f), 0.22f, gc);
    const Vector3 gb = w.ghostBlockPos();
    DrawCube(gb, w.block.half * 2, w.block.half * 2, w.block.half * 2, Color{130, 190, 255, 70});
    DrawCube({w.platePos.x, 0.08f, w.platePos.z}, 1.5f, 0.05f, 1.5f, Color{130, 255, 190, 140});
    lighting_.setEmissive(0.f);
  }

  EndShaderMode();
  EndMode3D();
}

void Renderer::drawDungeon(const World& w, Vector3 camPos) {
  const Dungeon& d = w.dungeon;
  const float c = cfg::kCell, H = cfg::kWallH;
  const float W = d.width() * c, D = d.depth() * c;
  DrawCube({W / 2, -0.1f, D / 2}, W, 0.2f, D, Color{70, 66, 62, 255});          // floor
  DrawCube({W / 2, H + 0.1f, D / 2}, W, 0.2f, D, Color{50, 48, 50, 255});       // ceiling

  for (int z = 0; z < d.depth(); ++z) {
    for (int x = 0; x < d.width(); ++x) {
      const Tile t = d.tile(x, z);
      if (t == Tile::Floor) continue;
      const Vector3 ctr = Dungeon::center(x, z);
      if (std::fabs(ctr.x - camPos.x) > 30.f || std::fabs(ctr.z - camPos.z) > 30.f) continue;
      const Vector3 mid{ctr.x, H / 2, ctr.z};
      if (t == Tile::Wall) {
        DrawCube(mid, c, H, c, wallColor(x, z));
        continue;
      }
      const bool open = (t == Tile::Door) ? d.doorsOpen : d.gatesOpen;
      if (open) continue;
      const bool alongX = d.tile(x - 1, z) == Tile::Wall && d.tile(x + 1, z) == Tile::Wall;
      if (t == Tile::Door) {
        DrawCube(mid, alongX ? c : 0.4f, H - 0.2f, alongX ? 0.4f : c, Color{110, 70, 35, 255});
      } else {  // iron gate: vertical bars
        for (int i = -2; i <= 2; ++i) {
          const Vector3 bar{ctr.x + (alongX ? i * 0.4f : 0.f), H / 2, ctr.z + (alongX ? 0.f : i * 0.4f)};
          DrawCube(bar, 0.12f, H - 0.4f, 0.12f, Color{70, 72, 80, 255});
        }
      }
    }
  }
}

void Renderer::drawPickup(const Pickup& pk, float time) {
  const Vector3 p{pk.pos.x, 0.9f + 0.15f * std::sin(time * 2.f), pk.pos.z};
  switch (pk.type) {
    case Pickup::Type::Key:
      DrawSphere(p, 0.18f, Color{235, 190, 50, 255});
      DrawCylinderEx(p, up(p, -0.5f), 0.05f, 0.05f, 6, Color{235, 190, 50, 255});
      DrawCube(up(p, -0.45f), 0.25f, 0.08f, 0.08f, Color{235, 190, 50, 255});
      break;
    case Pickup::Type::Potion:
      DrawSphere(p, 0.25f, Color{200, 40, 60, 255});
      DrawCylinderEx(up(p, 0.2f), up(p, 0.42f), 0.08f, 0.08f, 8, Color{190, 190, 200, 255});
      break;
    case Pickup::Type::Artifact:
      lighting_.setEmissive(0.8f);
      DrawSphere(up(p, 0.3f), 0.45f, Color{170, 90, 255, 255});
      DrawCylinderEx(up(p, -0.3f), up(p, 0.1f), 0.35f, 0.12f, 12, Color{90, 60, 140, 255});
      lighting_.setEmissive(0.f);
      break;
  }
}

void Renderer::drawPlayer(const Player& p, float time) {
  const bool flash = p.hurtTimer > 0.f;
  const Color cloth = flash ? Color{220, 60, 60, 255} : Color{60, 90, 160, 255};
  const Color skin{225, 190, 160, 255};
  const Vector3 f{std::sin(p.facing), 0.f, std::cos(p.facing)};
  const Vector3 r{-f.z, 0.f, f.x};
  const float swing = p.moving && p.onGround ? std::sin(time * 10.f) * 0.3f : 0.f;
  const Vector3 o = p.pos;

  DrawCylinderEx(add(o, {0.f, 0.85f, 0.f}), add(o, {0.f, 1.5f, 0.f}), 0.28f, 0.22f, 10, cloth);
  DrawSphere(add(o, {0.f, 1.78f, 0.f}), 0.22f, skin);
  for (int s = -1; s <= 1; s += 2) {  // legs
    const Vector3 hip{o.x + r.x * 0.14f * s, o.y + 0.85f, o.z + r.z * 0.14f * s};
    const Vector3 foot{hip.x + f.x * swing * s, o.y, hip.z + f.z * swing * s};
    DrawCylinderEx(hip, foot, 0.09f, 0.07f, 6, Color{50, 45, 40, 255});
  }
  // Sword arm sweeps across the front while attacking.
  const float prog = p.attackTimer > 0.f ? 1.f - p.attackTimer / 0.55f : 0.f;
  const float ang = p.facing + (p.attackTimer > 0.f ? (-1.f + 2.f * prog) * 1.2f : 0.4f);
  const Vector3 dir{std::sin(ang), 0.f, std::cos(ang)};
  const Vector3 hand{o.x + dir.x * 0.5f, o.y + 1.2f, o.z + dir.z * 0.5f};
  const Vector3 tip{o.x + dir.x * 1.5f, o.y + 1.25f, o.z + dir.z * 1.5f};
  DrawCylinderEx(add(o, {0.f, 1.35f, 0.f}), hand, 0.07f, 0.06f, 6, skin);
  DrawCylinderEx(hand, tip, 0.05f, 0.02f, 6, Color{200, 205, 215, 255});
}

void Renderer::drawSkeleton(const Enemy& e, float time) {
  const bool dead = e.state == EnemyState::Dead;
  const Color bone = e.hitFlash > 0.f ? Color{230, 80, 70, 255} : Color{222, 216, 195, 255};
  if (dead) {  // pile of bones
    DrawCylinder({e.pos.x, 0.f, e.pos.z}, 0.5f, 0.6f, 0.15f, 8, bone);
    DrawSphere({e.pos.x + 0.3f, 0.2f, e.pos.z}, 0.18f, bone);
    return;
  }
  if (hasSkeletonModel_) {
    lighting_.setModelMode(true);
    DrawModelEx(skeletonModel_, e.pos, {0.f, 1.f, 0.f}, e.yaw * kRad2Deg, {1.f, 1.f, 1.f}, bone);
    lighting_.setModelMode(false);
    return;
  }
  const Vector3 f{std::sin(e.yaw), 0.f, std::cos(e.yaw)};
  const Vector3 r{-f.z, 0.f, f.x};
  const float sw = e.moving ? std::sin(time * 9.f + e.home.x) * 0.3f : 0.f;
  const Vector3 o = e.pos;
  DrawCylinderEx(add(o, {0.f, 0.9f, 0.f}), add(o, {0.f, 1.55f, 0.f}), 0.07f, 0.07f, 6, bone);  // spine
  for (int i = 0; i < 3; ++i) {                                                                 // ribs
    const float y = 1.15f + 0.15f * i;
    DrawCylinderEx({o.x + r.x * 0.26f, o.y + y, o.z + r.z * 0.26f}, {o.x - r.x * 0.26f, o.y + y, o.z - r.z * 0.26f}, 0.04f, 0.04f, 6, bone);
  }
  DrawSphere(add(o, {0.f, 1.8f, 0.f}), 0.2f, bone);                                             // skull
  lighting_.setEmissive(1.f);
  DrawSphere({o.x + f.x * 0.16f + r.x * 0.07f, o.y + 1.83f, o.z + f.z * 0.16f + r.z * 0.07f}, 0.035f, RED);
  DrawSphere({o.x + f.x * 0.16f - r.x * 0.07f, o.y + 1.83f, o.z + f.z * 0.16f - r.z * 0.07f}, 0.035f, RED);
  lighting_.setEmissive(0.f);
  for (int s = -1; s <= 1; s += 2) {                                                            // legs
    const Vector3 hip{o.x + r.x * 0.12f * s, o.y + 0.9f, o.z + r.z * 0.12f * s};
    DrawCylinderEx(hip, {hip.x + f.x * sw * s, o.y, hip.z + f.z * sw * s}, 0.05f, 0.04f, 6, bone);
  }
  const Vector3 shoulder{o.x + r.x * 0.3f, o.y + 1.5f, o.z + r.z * 0.3f};                       // sword arm
  const float lunge = e.state == EnemyState::Attack ? 0.7f : 0.3f;
  const Vector3 hand{shoulder.x + f.x * lunge, o.y + 1.2f, shoulder.z + f.z * lunge};
  DrawCylinderEx(shoulder, hand, 0.04f, 0.04f, 6, bone);
  DrawCylinderEx(hand, {hand.x + f.x * 0.9f, hand.y + 0.1f, hand.z + f.z * 0.9f}, 0.04f, 0.015f, 6, Color{140, 140, 150, 255});
}
