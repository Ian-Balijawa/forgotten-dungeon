#pragma once
#include <raylib.h>
#include <cmath>
#include <string>
#include <vector>

#include "core/Config.hpp"

enum class Tile : unsigned char
{
  Floor,
  Wall,
  Door,
  Gate
};

// Grid-based level. Legend:  # wall  . floor  P start  E skeleton  K key  H potion
//                            D locked door  G gate (opens via plate)  S pushable block
//                            T pressure plate  A ancient artifact (goal)
class Dungeon
{
public:
  struct Spawns
  {
    Vector3 player{};
    std::vector<Vector3> enemies, keys, potions, artifacts, blocks, plates;
  };

  void load(const std::vector<std::string> &rows);
  int width() const { return width_; }
  int depth() const { return depth_; }
  Tile tile(int cx, int cz) const;
  bool solidCell(int cx, int cz) const;
  bool solidAt(float x, float z) const { return solidCell(cellOf(x), cellOf(z)); }
  bool circleHits(float x, float z, float r) const;

  static int cellOf(float v) { return static_cast<int>(std::floor(v / cfg::kCell)); }
  static Vector3 center(int cx, int cz) { return {(cx + 0.5f) * cfg::kCell, 0.f, (cz + 0.5f) * cfg::kCell}; }

  bool doorsOpen = false;
  bool gatesOpen = false;
  Spawns spawns;
  std::vector<Vector3> doors; // centres of locked doors

private:
  int width_ = 0, depth_ = 0;
  std::vector<Tile> tiles_;
};
