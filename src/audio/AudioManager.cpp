#include "audio/AudioManager.hpp"

#include <algorithm>

#include "audio/SoundSynth.hpp"

AudioManager::AudioManager() {
  InitAudioDevice();
  ready_ = IsAudioDeviceReady();
  if (!ready_) return;
  for (std::size_t i = 0; i < sounds_.size(); ++i) {
    const Wave w = synth::generate(static_cast<Sfx>(i));
    sounds_[i] = LoadSoundFromWave(w);
    UnloadWave(w);
    baseVolume_[i] = synth::defaultVolume(static_cast<Sfx>(i));
  }
  const Wave amb = synth::ambience();
  ambience_ = LoadSoundFromWave(amb);
  UnloadWave(amb);
  SetSoundVolume(ambience_, 0.35f);
}

AudioManager::~AudioManager() {
  if (!ready_) return;
  for (auto& s : sounds_) UnloadSound(s);
  UnloadSound(ambience_);
  CloseAudioDevice();
}

void AudioManager::play(Sfx id, float volume, float pitchJitter) {
  if (!ready_) return;
  const auto i = static_cast<std::size_t>(id);
  SetSoundVolume(sounds_[i], std::clamp(baseVolume_[i] * volume, 0.f, 1.f));
  SetSoundPitch(sounds_[i], 1.f + pitchJitter * static_cast<float>(GetRandomValue(-100, 100)) / 100.f);
  PlaySound(sounds_[i]);
}

void AudioManager::updateAmbience() {
  if (ready_ && !IsSoundPlaying(ambience_)) PlaySound(ambience_);
}
