#include "world/World.hpp"

#include <algorithm>
#include <cmath>

#include "physics/Collision.hpp"
#include "world/Level.hpp"

namespace {
constexpr float kPlayerR = 0.4f, kEnemyR = 0.45f, kSpeed = 5.f;
float dist2D(Vector3 a, Vector3 b) { return std::hypot(a.x - b.x, a.z - b.z); }
}  // namespace

World::World(AudioManager& audio) : audio_(audio) { reset(); }

void World::reset() {
  dungeon.load(kLevel1);
  player = Player{};
  player.pos = dungeon.spawns.player;
  enemies.clear();
  for (const auto& p : dungeon.spawns.enemies) {
    Enemy e;
    e.pos = e.home = e.lastSeen = p;
    e.timer = 1.f;
    enemies.push_back(e);
  }
  pickups.clear();
  for (const auto& p : dungeon.spawns.keys) pickups.push_back({Pickup::Type::Key, p, false});
  for (const auto& p : dungeon.spawns.potions) pickups.push_back({Pickup::Type::Potion, p, false});
  for (const auto& p : dungeon.spawns.artifacts) pickups.push_back({Pickup::Type::Artifact, p, false});
  block = Block{};
  hasPlate = !dungeon.spawns.plates.empty() && !dungeon.spawns.blocks.empty();
  if (hasPlate) {
    block.pos = blockStart = dungeon.spawns.blocks.front();
    block.pos.y = block.half;
    platePos = dungeon.spawns.plates.front();
  }
  plateDown = won = dead = false;
  message.clear();
  prompt.clear();
  messageTimer = 0.f;
  say("Where am I...?  WASD move, mouse look, LMB sword, E interact, F torch, Q time echo", 6.f);
}

void World::say(std::string text, float seconds) {
  message = std::move(text);
  messageTimer = seconds;
}

void World::playAt(Sfx id, Vector3 pos, float volume) {
  const float v = std::clamp(1.f - dist2D(pos, player.pos) / 22.f, 0.f, 1.f);
  if (v > 0.03f) audio_.play(id, volume * v, 0.08f);
}

bool World::blockHitsWorld(Vector3 p) const {
  const float h = block.half;
  return dungeon.solidAt(p.x - h, p.z - h) || dungeon.solidAt(p.x + h, p.z - h) ||
         dungeon.solidAt(p.x - h, p.z + h) || dungeon.solidAt(p.x + h, p.z + h);
}

bool World::circleHitsBlock(float x, float z, float r) const {
  return hasPlate && physics::circleVsSquare(x, z, r, block.pos.x, block.pos.z, block.half);
}

bool World::lineOfSight(Vector3 a, Vector3 b) const {
  const float d = dist2D(a, b);
  const int steps = std::max(1, static_cast<int>(d / 0.5f));
  for (int i = 1; i < steps; ++i) {
    const float t = static_cast<float>(i) / steps;
    if (dungeon.solidAt(a.x + (b.x - a.x) * t, a.z + (b.z - a.z) * t)) return false;
  }
  return true;
}

Vector3 World::ghostPos() const {
  const float t = 1.f - player.echoTimer / 6.f;
  const float s = t * t * (3.f - 2.f * t);
  const float x0 = blockStart.x - 1.4f, x1 = platePos.x - 1.4f;
  return {x0 + (x1 - x0) * s, 0.f, blockStart.z + (platePos.z - blockStart.z) * s};
}
Vector3 World::ghostBlockPos() const {
  Vector3 g = ghostPos();
  return {g.x + 1.4f, block.half, g.z};
}

void World::update(float dt, const PlayerInput& in) {
  dt_ = dt;
  messageTimer = std::max(0.f, messageTimer - dt);
  if (messageTimer <= 0.f) message.clear();
  prompt.clear();
  updatePlayer(dt, in);
  updateBlock(dt);
  updatePickups();
  updatePlate();
  for (auto& e : enemies) updateEnemy(e, dt);
  if (player.hp <= 0 && !dead) {
    dead = true;
    audio_.play(Sfx::PlayerDeath);
  }
}

// ------------------------------------------------------------------ player
void World::movePlayerAxis(bool xAxis, float delta, bool& pushing) {
  if (delta == 0.f) return;
  Vector3 np = player.pos;
  (xAxis ? np.x : np.z) += delta;
  if (dungeon.circleHits(np.x, np.z, kPlayerR)) return;

  if (player.pos.y < 1.0f && circleHitsBlock(np.x, np.z, kPlayerR)) {
    // Heavier block => slower push (mass 8 -> ~55% speed). Player and block move together.
    const float pd = delta / (1.f + block.mass * 0.1f);
    np = player.pos;
    (xAxis ? np.x : np.z) += pd;
    Vector3 nb = block.pos;
    (xAxis ? nb.x : nb.z) += pd;
    if (blockHitsWorld(nb) || dungeon.circleHits(np.x, np.z, kPlayerR)) return;
    block.pos = nb;
    (xAxis ? block.vel.x : block.vel.z) = pd / std::max(dt_, 0.001f) * 0.8f;  // keeps sliding a little
    pushing = true;
  }
  player.pos = np;
}

void World::updatePlayer(float dt, const PlayerInput& in) {
  Player& p = player;
  p.attackTimer = std::max(0.f, p.attackTimer - dt);
  p.hurtTimer = std::max(0.f, p.hurtTimer - dt);
  p.echoTimer = std::max(0.f, p.echoTimer - dt);
  p.echoCooldown = std::max(0.f, p.echoCooldown - dt);

  if (in.torch) {
    p.torchOn = !p.torchOn;
    audio_.play(Sfx::TorchToggle);
    say(p.torchOn ? "Torch lit." : "Torch out - creatures see you less, but so do you.", 2.5f);
  }
  if (in.echo && hasPlate && p.echoCooldown <= 0.f) {
    p.echoTimer = 6.f;
    p.echoCooldown = 14.f;
    audio_.play(Sfx::EchoStart);
    say("The dungeon remembers...", 3.f);
  }
  if (in.drink && p.potions > 0 && p.hp < p.maxHp) {
    --p.potions;
    p.hp = std::min(p.maxHp, p.hp + 40);
    audio_.play(Sfx::Potion);
  }

  // Movement + collision (per axis so you slide along walls).
  bool pushing = false;
  p.moving = in.move.x != 0.f || in.move.y != 0.f;
  movePlayerAxis(true, in.move.x * kSpeed * dt, pushing);
  movePlayerAxis(false, in.move.y * kSpeed * dt, pushing);
  if (p.moving) p.facing = std::atan2(in.move.x, in.move.y);

  // Gravity + jump.
  if (in.jump && p.onGround) {
    p.vy = 7.5f;
    p.onGround = false;
    audio_.play(Sfx::Jump, 1.f, 0.05f);
  }
  p.vy -= physics::kGravity * dt;
  p.pos.y += p.vy * dt;
  if (p.pos.y <= 0.f) {
    if (!p.onGround && p.vy < -4.f) audio_.play(Sfx::Land);
    p.pos.y = 0.f;
    p.vy = 0.f;
    p.onGround = true;
  }

  // Footsteps.
  if (p.moving && p.onGround) {
    p.stepTimer -= dt;
    if (p.stepTimer <= 0.f) {
      audio_.play(Sfx::Footstep, 1.f, 0.12f);
      p.stepTimer = pushing ? 0.5f : 0.36f;
    }
  } else {
    p.stepTimer = 0.f;
  }

  // Sword.
  if (in.attack && p.attackTimer <= 0.f) {
    p.attackTimer = 0.55f;
    p.facing = in.yaw;
    audio_.play(Sfx::SwordSwing, 1.f, 0.08f);
    const float fx = std::sin(in.yaw), fz = std::cos(in.yaw);
    for (auto& e : enemies) {
      if (e.state == EnemyState::Dead) continue;
      const float d = dist2D(e.pos, p.pos);
      const float dot = d > 0.01f ? ((e.pos.x - p.pos.x) * fx + (e.pos.z - p.pos.z) * fz) / d : 1.f;
      if (d < 2.8f && (d < 0.8f || dot > 0.35f)) hitEnemy(e, 25);
    }
  }

  // Doors.
  if (!dungeon.doorsOpen) {
    for (const auto& dpos : dungeon.doors) {
      if (dist2D(dpos, p.pos) > 3.0f) continue;
      prompt = p.hasKey ? "E - unlock door" : "Locked. Find a key.";
      if (in.interact) {
        if (p.hasKey) {
          dungeon.doorsOpen = true;
          audio_.play(Sfx::KeyUse);
          audio_.play(Sfx::DoorOpen);
          say("The door groans open.", 3.f);
        } else {
          audio_.play(Sfx::DoorLocked);
          say("It's locked.", 2.f);
        }
      }
      break;
    }
  }
}

void World::hitEnemy(Enemy& e, int dmg) {
  e.hp -= dmg;
  e.hitFlash = 0.2f;
  audio_.play(Sfx::SwordHit, 1.f, 0.08f);
  if (e.hp <= 0) {
    e.state = EnemyState::Dead;
    playAt(Sfx::EnemyDeath, e.pos);
  } else {
    playAt(Sfx::EnemyHurt, e.pos);
    e.state = EnemyState::Chase;
  }
}

// ------------------------------------------------------------------ physics object
void World::updateBlock(float dt) {
  if (!hasPlate) return;
  block.soundTimer = std::max(0.f, block.soundTimer - dt);
  const float speed = std::hypot(block.vel.x, block.vel.z);
  if (speed > 0.f) {
    Vector3 nb = block.pos;
    nb.x += block.vel.x * dt;
    nb.z += block.vel.z * dt;
    if (blockHitsWorld(nb)) block.vel = {};
    else block.pos = nb;
    const float k = std::exp(-7.f * dt);  // friction
    block.vel.x *= k;
    block.vel.z *= k;
    if (speed < 0.05f) block.vel = {};
  }
  if (speed > 0.5f && block.soundTimer <= 0.f) {
    playAt(Sfx::StoneDrag, block.pos);
    block.soundTimer = 0.32f;
  }
}

void World::updatePlate() {
  if (!hasPlate || plateDown) return;
  if (std::fabs(block.pos.x - platePos.x) < 0.8f && std::fabs(block.pos.z - platePos.z) < 0.8f) {
    plateDown = true;
    dungeon.gatesOpen = true;
    playAt(Sfx::PlateClick, platePos);
    audio_.play(Sfx::GateOpen);
    say("Somewhere, stone grinds against stone...", 4.f);
  }
}

void World::updatePickups() {
  for (auto& pk : pickups) {
    if (pk.taken || dist2D(pk.pos, player.pos) > 1.3f) continue;
    pk.taken = true;
    switch (pk.type) {
      case Pickup::Type::Key:
        player.hasKey = true;
        audio_.play(Sfx::Pickup);
        say("Picked up a rusted key.", 3.f);
        break;
      case Pickup::Type::Potion:
        ++player.potions;
        audio_.play(Sfx::Pickup);
        say("Health potion collected (R to drink).", 3.f);
        break;
      case Pickup::Type::Artifact:
        won = true;
        audio_.play(Sfx::Win);
        break;
    }
  }
}

// ------------------------------------------------------------------ enemy AI (finite state machine)
void World::moveEnemy(Enemy& e, float dx, float dz, float speed, float dt) {
  const float len = std::hypot(dx, dz);
  if (len < 0.001f) { e.moving = false; return; }
  const float sx = dx / len * speed * dt, sz = dz / len * speed * dt;
  bool moved = false;
  if (!dungeon.circleHits(e.pos.x + sx, e.pos.z, kEnemyR) && !circleHitsBlock(e.pos.x + sx, e.pos.z, kEnemyR)) {
    e.pos.x += sx; moved = true;
  }
  if (!dungeon.circleHits(e.pos.x, e.pos.z + sz, kEnemyR) && !circleHitsBlock(e.pos.x, e.pos.z + sz, kEnemyR)) {
    e.pos.z += sz; moved = true;
  }
  e.yaw = std::atan2(dx, dz);
  e.moving = moved;
}

void World::updateEnemy(Enemy& e, float dt) {
  e.hitFlash = std::max(0.f, e.hitFlash - dt);
  if (e.state == EnemyState::Dead) { e.deathTimer += dt; return; }
  e.attackCd = std::max(0.f, e.attackCd - dt);
  e.moving = false;

  const float d = dist2D(e.pos, player.pos);
  const float sight = player.torchOn ? 11.f : 4.5f;  // the torch makes you easier to spot
  const float hearing = player.moving ? 6.f : 2.f;   // footsteps give you away
  const bool sees = d < sight && lineOfSight(e.pos, player.pos);
  const bool hears = d < hearing;
  const bool aware = (sees || hears) && player.hp > 0;

  switch (e.state) {
    case EnemyState::Idle:
      e.timer -= dt;
      if (aware) { e.state = EnemyState::Chase; playAt(Sfx::EnemyAlert, e.pos); }
      else if (e.timer <= 0.f) {
        e.state = EnemyState::Patrol;
        e.timer = 2.f + static_cast<float>(GetRandomValue(0, 200)) / 100.f;
        e.yaw = static_cast<float>(GetRandomValue(0, 628)) / 100.f;
      }
      break;
    case EnemyState::Patrol: {
      e.timer -= dt;
      float dx = std::sin(e.yaw), dz = std::cos(e.yaw);
      if (dist2D(e.pos, e.home) > 5.f) { dx = e.home.x - e.pos.x; dz = e.home.z - e.pos.z; }
      moveEnemy(e, dx, dz, 1.3f, dt);
      if (!e.moving) e.yaw += 1.5f;
      if (aware) { e.state = EnemyState::Chase; playAt(Sfx::EnemyAlert, e.pos); }
      else if (e.timer <= 0.f) { e.state = EnemyState::Idle; e.timer = 1.5f; }
      break;
    }
    case EnemyState::Chase:
      if (sees) e.lastSeen = player.pos;
      moveEnemy(e, player.pos.x - e.pos.x, player.pos.z - e.pos.z, 3.2f, dt);
      if (d < 1.6f) e.state = EnemyState::Attack;
      else if (!aware) { e.state = EnemyState::Search; e.timer = 4.f; }
      break;
    case EnemyState::Attack:
      e.yaw = std::atan2(player.pos.x - e.pos.x, player.pos.z - e.pos.z);
      if (d > 2.0f) { e.state = EnemyState::Chase; break; }
      if (e.attackCd <= 0.f) {
        e.attackCd = 1.3f;
        playAt(Sfx::EnemyAttack, e.pos);
        if (player.hurtTimer <= 0.f && player.hp > 0) {
          player.hp -= 10;
          player.hurtTimer = 0.4f;
          audio_.play(Sfx::PlayerHurt, 1.f, 0.05f);
        }
      }
      break;
    case EnemyState::Search:
      e.timer -= dt;
      moveEnemy(e, e.lastSeen.x - e.pos.x, e.lastSeen.z - e.pos.z, 2.2f, dt);
      if (aware) { e.state = EnemyState::Chase; playAt(Sfx::EnemyAlert, e.pos); }
      else if (e.timer <= 0.f) { e.state = EnemyState::Idle; e.timer = 1.f; }
      break;
    case EnemyState::Dead: break;
  }

  if (e.moving) {
    e.stepTimer -= dt;
    if (e.stepTimer <= 0.f) {
      playAt(Sfx::BoneStep, e.pos, 1.f);
      e.stepTimer = e.state == EnemyState::Chase ? 0.25f : 0.5f;
    }
  }
}
