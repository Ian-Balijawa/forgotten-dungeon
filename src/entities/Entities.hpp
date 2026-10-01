#pragma once
#include <raylib.h>

enum class EnemyState { Idle, Patrol, Chase, Attack, Search, Dead };

struct Player {
  Vector3 pos{};
  float vy = 0.f;
  bool onGround = true, moving = false;
  float facing = 0.f;  // yaw, radians
  int hp = 100, maxHp = 100, potions = 0;
  bool torchOn = true, hasKey = false;
  float attackTimer = 0.f, hurtTimer = 0.f, stepTimer = 0.f;
  float echoTimer = 0.f, echoCooldown = 0.f;
};

struct Enemy {
  Vector3 pos{}, home{}, lastSeen{};
  float yaw = 0.f, timer = 0.f, attackCd = 0.f, stepTimer = 0.f, hitFlash = 0.f, deathTimer = 0.f;
  int hp = 50;
  EnemyState state = EnemyState::Idle;
  bool moving = false;
};

// Rigid body the player can shove onto the pressure plate.
struct Block {
  Vector3 pos{}, vel{};
  float half = 0.85f;
  float mass = 8.f;
  float soundTimer = 0.f;
};

struct Pickup {
  enum class Type { Key, Potion, Artifact } type = Type::Key;
  Vector3 pos{};
  bool taken = false;
};
