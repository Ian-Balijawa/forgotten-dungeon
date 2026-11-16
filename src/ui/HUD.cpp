#include "ui/HUD.hpp"

#include <algorithm>

namespace
{
  void centered(const char *text, int y, int size, Color c)
  {
    DrawText(text, (GetScreenWidth() - MeasureText(text, size)) / 2, y, size, c);
  }
} // namespace

namespace hud
{

  void menu()
  {
    const int h = GetScreenHeight();
    centered("THE FORGOTTEN DUNGEON", h / 2 - 140, 56, Color{230, 180, 90, 255});
    centered("a dungeon that remembers", h / 2 - 70, 24, GRAY);
    centered("Press ENTER to begin", h / 2 + 10, 28, RAYWHITE);
    centered("WASD move  |  Mouse look  |  Space jump  |  LMB sword  |  E interact", h / 2 + 80, 20, GRAY);
    centered("F torch  |  Q time echo  |  R potion  |  V first-person  |  ESC pause", h / 2 + 108, 20, GRAY);
    centered("Tip: skeletons hear your footsteps and see your torch from far away.", h / 2 + 150, 18, DARKGRAY);
  }

  void playing(const World &w, bool firstPerson)
  {
    const Player &p = w.player;
    const int sw = GetScreenWidth(), sh = GetScreenHeight();

    DrawRectangle(20, 20, 244, 24, Color{0, 0, 0, 150});
    DrawRectangle(22, 22, 240 * std::max(0, p.hp) / p.maxHp, 20, Color{190, 40, 50, 255});
    DrawText("HEALTH", 28, 25, 16, RAYWHITE);

    DrawText(TextFormat("Torch: %s  (F)", p.torchOn ? "ON" : "OFF"), 20, 54, 18, p.torchOn ? ORANGE : GRAY);
    DrawText(TextFormat("Key: %s", p.hasKey ? "yes" : "no"), 20, 76, 18, p.hasKey ? GOLD : GRAY);
    DrawText(TextFormat("Potions: %d  (R)", p.potions), 20, 98, 18, p.potions ? RED : GRAY);

    const float echoReady = p.echoCooldown <= 0.f ? 1.f : 1.f - p.echoCooldown / 14.f;
    DrawText("Time echo (Q)", 20, 124, 18, SKYBLUE);
    DrawRectangle(20, 146, 150, 8, Color{0, 0, 0, 150});
    DrawRectangle(20, 146, static_cast<int>(150 * echoReady), 8, Color{110, 180, 255, 255});

    if (firstPerson)
      DrawCircle(sw / 2, sh / 2, 3.f, Color{255, 255, 255, 180});
    if (!w.prompt.empty())
      centered(w.prompt.c_str(), sh - 140, 24, RAYWHITE);
    if (!w.message.empty())
      centered(w.message.c_str(), sh - 90, 22, Color{240, 220, 170, 255});
    if (p.hurtTimer > 0.f)
      DrawRectangle(0, 0, sw, sh, Color{200, 0, 0, static_cast<unsigned char>(90 * p.hurtTimer / 0.4f)});
    DrawFPS(sw - 90, 10);
  }

  void overlay(const char *title, const char *subtitle, Color color)
  {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, 170});
    centered(title, GetScreenHeight() / 2 - 50, 60, color);
    centered(subtitle, GetScreenHeight() / 2 + 30, 22, RAYWHITE);
  }

} // namespace hud
