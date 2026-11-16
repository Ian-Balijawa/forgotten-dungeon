#pragma once
#include <raylib.h>
#include <filesystem>
#include <string>
#include <vector>

// Finds an asset in dev (./assets), next to the exe, or in the installed locations
// of each platform (Linux /usr/share, macOS .app/Contents/Resources, Windows ./assets).
inline std::string assetPath(const std::string &rel)
{
  namespace fs = std::filesystem;
  const fs::path app = GetApplicationDirectory();
  const std::vector<fs::path> roots = {
      fs::path("assets"), app / "assets", app / ".." / "Resources" / "assets",
      app / ".." / "share" / "forgotten-dungeon" / "assets"};
  for (const auto &r : roots)
  {
    std::error_code ec;
    if (fs::exists(r / rel, ec))
      return (r / rel).string();
  }
  return {};
}
