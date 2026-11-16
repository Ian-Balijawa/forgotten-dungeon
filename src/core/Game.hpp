#pragma once
#include "audio/AudioManager.hpp"
#include "rendering/CameraRig.hpp"
#include "rendering/Lighting.hpp"
#include "rendering/Renderer.hpp"
#include "world/World.hpp"

enum class GameState
{
  Menu,
  Playing,
  Paused,
  Dead,
  Won
};

class Game
{
public:
  Game();
  void run();

private:
  struct WindowGuard
  { // must be constructed first / destroyed last
    WindowGuard();
    ~WindowGuard();
  } window_;

  void update(float dt);
  void draw();
  void startNewGame();
  PlayerInput readInput();

  AudioManager audio_;
  World world_;
  Lighting lighting_;
  Renderer renderer_;
  CameraRig camera_;
  GameState state_ = GameState::Menu;
  bool quit_ = false;
  float time_ = 0.f;
};
