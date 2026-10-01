# The Forgotten Dungeon

A small 3D dungeon adventure in modern C++20 built on [raylib](https://www.raylib.com/).
Physics (gravity, friction, a pushable stone block), a torch with per-pixel lighting, third/first-person
camera with wall collision, finite-state-machine skeleton AI, inventory, a "time echo" ghost that reveals
the puzzle, and **full procedural audio** (every action has a sound, no audio files needed).

## Play (from source, macOS)

```bash
brew install cmake        # raylib is downloaded and built automatically
make run                  # configure + build + launch
```

Linux: `make deps && make run`. Windows: `cmake -S . -B build && cmake --build build --config Release`.

## Controls

| Key | Action | Key | Action |
|---|---|---|---|
| WASD | move | LMB | sword |
| Mouse | look | E | interact (doors) |
| Space | jump | F | torch on/off |
| Q | time echo | R | drink potion |
| V | first/third person | ESC / P | pause |

Goal: key -> door -> push the stone block east onto the pressure plate -> gate opens -> take the artifact.
Skeletons hear footsteps and see your torch from far away; put it out to sneak.

## Installers

```bash
make package      # macOS -> build/ForgottenDungeon-1.0.0-macOS.dmg
                  # Linux -> .deb + .tar.gz      Windows -> run: cd build && cpack -C Release (NSIS .exe + .zip)
```

`.github/workflows/release.yml` builds all three on GitHub's runners. Tag a version and the installers
are attached to a GitHub Release:

```bash
git tag v1.0.0 && git push origin v1.0.0
```

Notes: macOS builds are unsigned (first launch: right-click -> Open); for public distribution add an Apple
Developer ID signature + notarization. Windows installers are likewise unsigned (SmartScreen warning).

## Layout

```
CMakeLists.txt  Makefile  cmake/Packaging.cmake  .github/workflows/release.yml
assets/            optional models/textures/sounds (game runs without any)
packaging/linux/   .desktop entry
src/
  main.cpp
  core/       Game loop + state machine, Config, asset path lookup
  audio/      AudioManager, SoundSynth (procedural SFX + ambience)
  rendering/  Renderer, Lighting (GLSL torch + fog), CameraRig
  physics/    collision helpers
  entities/   Player, Enemy, Block, Pickup data
  world/      Dungeon grid, Level map, World (rules + enemy AI)
  ui/         HUD
```

Edit `src/world/Level.hpp` to design new maps (legend in `Dungeon.hpp`).
