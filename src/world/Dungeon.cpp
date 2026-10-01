#include "world/Dungeon.hpp"

#include <algorithm>

void Dungeon::load(const std::vector<std::string>& rows) {
  depth_ = static_cast<int>(rows.size());
  width_ = 0;
  for (const auto& r : rows) width_ = std::max(width_, static_cast<int>(r.size()));
  tiles_.assign(static_cast<size_t>(width_ * depth_), Tile::Wall);
  spawns = {};
  doors.clear();
  doorsOpen = gatesOpen = false;

  for (int z = 0; z < depth_; ++z) {
    for (int x = 0; x < width_; ++x) {
      const char c = x < static_cast<int>(rows[z].size()) ? rows[z][x] : '#';
      const Vector3 p = center(x, z);
      Tile t = Tile::Floor;
      switch (c) {
        case '#': t = Tile::Wall; break;
        case 'D': t = Tile::Door; doors.push_back(p); break;
        case 'G': t = Tile::Gate; break;
        case 'P': spawns.player = p; break;
        case 'E': spawns.enemies.push_back(p); break;
        case 'K': spawns.keys.push_back(p); break;
        case 'H': spawns.potions.push_back(p); break;
        case 'A': spawns.artifacts.push_back(p); break;
        case 'S': spawns.blocks.push_back(p); break;
        case 'T': spawns.plates.push_back(p); break;
        default: break;
      }
      tiles_[static_cast<size_t>(z * width_ + x)] = t;
    }
  }
}

Tile Dungeon::tile(int cx, int cz) const {
  if (cx < 0 || cz < 0 || cx >= width_ || cz >= depth_) return Tile::Wall;
  return tiles_[static_cast<size_t>(cz * width_ + cx)];
}

bool Dungeon::solidCell(int cx, int cz) const {
  switch (tile(cx, cz)) {
    case Tile::Wall: return true;
    case Tile::Door: return !doorsOpen;
    case Tile::Gate: return !gatesOpen;
    default: return false;
  }
}

bool Dungeon::circleHits(float x, float z, float r) const {
  return solidAt(x - r, z - r) || solidAt(x + r, z - r) || solidAt(x - r, z + r) || solidAt(x + r, z + r);
}
