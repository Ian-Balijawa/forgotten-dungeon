#pragma once
#include <raylib.h>
#include <array>
#include <cstddef>

enum class Sfx {
  Footstep, BoneStep, SwordSwing, SwordHit, EnemyHurt, EnemyDeath, EnemyAlert, EnemyAttack,
  PlayerHurt, PlayerDeath, Jump, Land, Pickup, Potion, KeyUse, DoorOpen, DoorLocked,
  StoneDrag, PlateClick, GateOpen, TorchToggle, EchoStart, Win, UiClick, Count
};

// Owns every sound. All effects are synthesised at startup (see SoundSynth.cpp),
// so the game has full audio with no sound files to ship.
class AudioManager {
 public:
  AudioManager();
  ~AudioManager();
  AudioManager(const AudioManager&) = delete;
  AudioManager& operator=(const AudioManager&) = delete;

  void play(Sfx id, float volume = 1.f, float pitchJitter = 0.f);
  void updateAmbience();  // keeps the dungeon drone looping
  bool ready() const { return ready_; }

 private:
  bool ready_ = false;
  std::array<Sound, static_cast<std::size_t>(Sfx::Count)> sounds_{};
  Sound ambience_{};
  std::array<float, static_cast<std::size_t>(Sfx::Count)> baseVolume_{};
};
