#include "core/Game.hpp"

#include <cmath>

#include "core/Config.hpp"
#include "ui/HUD.hpp"

Game::WindowGuard::WindowGuard()
{
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(cfg::kWindowW, cfg::kWindowH, cfg::kTitle);
  SetExitKey(KEY_NULL); // ESC pauses instead of quitting
  SetTargetFPS(120);
}
Game::WindowGuard::~WindowGuard() { CloseWindow(); }

Game::Game() : world_(audio_), renderer_(lighting_) {}

void Game::startNewGame()
{
  world_.reset();
  camera_ = CameraRig();
  state_ = GameState::Playing;
  DisableCursor();
  audio_.play(Sfx::UiClick);
}

PlayerInput Game::readInput()
{
  PlayerInput in;
  const float yaw = camera_.yaw();
  const Vector2 fwd{sinf(yaw), cosf(yaw)};
  const Vector2 right{-cosf(yaw), sinf(yaw)};
  const float f = (IsKeyDown(KEY_W) ? 1.f : 0.f) - (IsKeyDown(KEY_S) ? 1.f : 0.f);
  const float r = (IsKeyDown(KEY_D) ? 1.f : 0.f) - (IsKeyDown(KEY_A) ? 1.f : 0.f);
  Vector2 m{fwd.x * f + right.x * r, fwd.y * f + right.y * r};
  const float len = sqrtf(m.x * m.x + m.y * m.y);
  if (len > 1.f)
  {
    m.x /= len;
    m.y /= len;
  }
  in.move = m;
  in.yaw = yaw;
  in.jump = IsKeyPressed(KEY_SPACE);
  in.attack = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  in.interact = IsKeyPressed(KEY_E);
  in.torch = IsKeyPressed(KEY_F);
  in.echo = IsKeyPressed(KEY_Q);
  in.drink = IsKeyPressed(KEY_R);
  return in;
}

void Game::update(float dt)
{
  audio_.updateAmbience();
  switch (state_)
  {
  case GameState::Menu:
    if (IsKeyPressed(KEY_ENTER))
      startNewGame();
    if (IsKeyPressed(KEY_ESCAPE))
      quit_ = true;
    break;
  case GameState::Playing:
  {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
    {
      state_ = GameState::Paused;
      EnableCursor();
      break;
    }
    camera_.look(GetMouseDelta());
    if (IsKeyPressed(KEY_V))
      camera_.toggleFirstPerson();
    world_.update(dt, readInput());
    camera_.update(world_.player.pos, world_.dungeon);
    if (world_.won || world_.dead)
    {
      state_ = world_.won ? GameState::Won : GameState::Dead;
      EnableCursor();
    }
    break;
  }
  case GameState::Paused:
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P))
    {
      state_ = GameState::Playing;
      DisableCursor();
    }
    if (IsKeyPressed(KEY_Q))
    {
      state_ = GameState::Menu;
    }
    break;
  case GameState::Dead:
  case GameState::Won:
    if (IsKeyPressed(KEY_ENTER))
      startNewGame();
    if (IsKeyPressed(KEY_ESCAPE))
      state_ = GameState::Menu;
    break;
  }
}

void Game::draw()
{
  BeginDrawing();
  ClearBackground(Color{5, 5, 8, 255});
  if (state_ != GameState::Menu)
    renderer_.drawWorld(world_, camera_, time_);
  switch (state_)
  {
  case GameState::Menu:
    hud::menu();
    break;
  case GameState::Playing:
    hud::playing(world_, camera_.firstPerson());
    break;
  case GameState::Paused:
    hud::playing(world_, camera_.firstPerson());
    hud::overlay("PAUSED", "ESC / P resume   -   Q main menu", RAYWHITE);
    break;
  case GameState::Dead:
    hud::overlay("YOU DIED", "ENTER try again   -   ESC menu", Color{200, 40, 40, 255});
    break;
  case GameState::Won:
    hud::overlay("YOU ESCAPED", "The dungeon remembers you.  ENTER play again   -   ESC menu",
                 Color{240, 200, 90, 255});
    break;
  }
  EndDrawing();
}

void Game::run()
{
  while (!quit_ && !WindowShouldClose())
  {
    const float dt = fminf(GetFrameTime(), 0.05f);
    time_ += dt;
    update(dt);
    draw();
  }
}
