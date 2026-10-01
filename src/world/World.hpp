#pragma once
#include <raylib.h>
#include <string>
#include <vector>

#include "audio/AudioManager.hpp"
#include "entities/Entities.hpp"
#include "world/Dungeon.hpp"

struct PlayerInput {
  Vector2 move{};  // world-space XZ direction, length <= 1
  float yaw = 0.f;
  bool jump = false, attack = false, interact = false, torch = false, echo = false, drink = false;
};

// All game rules and simulation. Rendering lives in rendering/, so this stays testable.
class World {
 public:
  explicit World(AudioManager& audio);
  void reset();
  void update(float dt, const PlayerInput& in);
  void say(std::string text, float seconds = 3.f);

  // "Time echo": a ghost replays how the block was pushed 500 years ago.
  bool echoActive() const { return player.echoTimer > 0.f; }
  Vector3 ghostPos() const;
  Vector3 ghostBlockPos() const;

  Dungeon dungeon;
  Player player;
  std::vector<Enemy> enemies;
  std::vector<Pickup> pickups;
  Block block;
  Vector3 blockStart{}, platePos{};
  bool hasPlate = false, plateDown = false, won = false, dead = false;
  std::string message, prompt;
  float messageTimer = 0.f;

 private:
  void updatePlayer(float dt, const PlayerInput& in);
  void movePlayerAxis(bool xAxis, float delta, bool& pushing);
  void updateBlock(float dt);
  void updatePickups();
  void updatePlate();
  void updateEnemy(Enemy& e, float dt);
  void moveEnemy(Enemy& e, float dx, float dz, float speed, float dt);
  void hitEnemy(Enemy& e, int dmg);
  bool lineOfSight(Vector3 a, Vector3 b) const;
  bool blockHitsWorld(Vector3 p) const;
  bool circleHitsBlock(float x, float z, float r) const;
  void playAt(Sfx id, Vector3 pos, float volume = 1.f);

  AudioManager& audio_;
  float dt_ = 0.016f;
};
