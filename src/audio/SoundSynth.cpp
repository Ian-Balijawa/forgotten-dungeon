#include "audio/SoundSynth.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

namespace {
constexpr int kRate = 44100;
constexpr float kPi = 3.14159265358979f;

float rnd() {  // white noise in [-1, 1]
  static uint32_t s = 0x12345678u;
  s ^= s << 13; s ^= s >> 17; s ^= s << 5;
  return (s & 0xFFFFFF) / 8388607.5f - 1.f;
}
float sine(float f, float t) { return sinf(2.f * kPi * f * t); }
float chirp(float f0, float f1, float t, float d) {  // linear frequency sweep
  return sinf(2.f * kPi * (f0 * t + (f1 - f0) * t * t / (2.f * d)));
}
float saw(float f, float t) { float p = f * t; return 2.f * (p - floorf(p + 0.5f)); }
float decay(float t, float k) { return expf(-k * t); }
struct LowPass {
  float y = 0.f;
  float operator()(float x, float a) { y += a * (x - y); return y; }
};

using Fn = std::function<float(float, float)>;
Wave render(float seconds, const Fn& fn) {
  const int n = static_cast<int>(seconds * kRate);
  auto* data = static_cast<int16_t*>(MemAlloc(static_cast<unsigned>(n) * sizeof(int16_t)));
  for (int i = 0; i < n; ++i) {
    const float t = static_cast<float>(i) / kRate;
    float s = fn(t, seconds);
    const float fade = std::min(1.f, (seconds - t) * 200.f);  // avoid end clicks
    s = std::clamp(s * fade, -1.f, 1.f);
    data[i] = static_cast<int16_t>(s * 30000.f);
  }
  Wave w{};
  w.frameCount = static_cast<unsigned>(n);
  w.sampleRate = kRate;
  w.sampleSize = 16;
  w.channels = 1;
  w.data = data;
  return w;
}
}  // namespace

namespace synth {

float defaultVolume(Sfx id) {
  switch (id) {
    case Sfx::Footstep: return 0.45f;
    case Sfx::BoneStep: return 0.5f;
    case Sfx::StoneDrag: return 0.7f;
    case Sfx::GateOpen: return 0.9f;
    case Sfx::Win: return 0.9f;
    default: return 0.8f;
  }
}

Wave generate(Sfx id) {
  switch (id) {
    case Sfx::Footstep:
      return render(0.16f, [lp = LowPass{}](float t, float) mutable {
        return (lp(rnd(), 0.12f) * 1.8f + sine(60.f, t) * 0.6f) * decay(t, 32.f);
      });
    case Sfx::BoneStep:
      return render(0.12f, [lp = LowPass{}](float t, float) mutable {
        return (lp(rnd(), 0.5f) * 0.9f + sine(900.f, t) * 0.2f) * decay(t, 45.f);
      });
    case Sfx::SwordSwing:
      return render(0.3f, [lp = LowPass{}](float t, float d) mutable {
        return lp(rnd(), 0.04f + 0.5f * (t / d)) * sinf(kPi * t / d) * 1.6f;
      });
    case Sfx::SwordHit:
      return render(0.3f, [](float t, float d) {
        return chirp(180.f, 60.f, t, d) * 0.7f * decay(t, 10.f) + rnd() * 0.5f * decay(t, 45.f) +
               sine(1400.f, t) * 0.25f * decay(t, 28.f) + sine(2100.f, t) * 0.15f * decay(t, 35.f);
      });
    case Sfx::EnemyHurt:
      return render(0.3f, [](float t, float d) {
        return saw(220.f - 110.f * t / d, t) * 0.35f * decay(t, 6.f);
      });
    case Sfx::EnemyDeath:
      return render(0.9f, [lp = LowPass{}](float t, float d) mutable {
        const float rattle = (sinf(60.f * t) > 0.f ? 1.f : 0.f) * lp(rnd(), 0.4f) * 0.5f;
        return (chirp(150.f, 40.f, t, d) * 0.5f + rattle) * decay(t, 3.f);
      });
    case Sfx::EnemyAlert:
      return render(0.5f, [](float t, float d) {
        return saw(90.f + 50.f * t / d, t) * (0.6f + 0.4f * sine(14.f, t)) * 0.4f * sinf(kPi * t / d);
      });
    case Sfx::EnemyAttack:
      return render(0.22f, [lp = LowPass{}](float t, float d) mutable {
        return lp(rnd(), 0.08f + 0.3f * t / d) * sinf(kPi * t / d) * 1.2f + sine(110.f, t) * 0.2f * decay(t, 12.f);
      });
    case Sfx::PlayerHurt:
      return render(0.3f, [](float t, float d) {
        return (chirp(260.f, 90.f, t, d) * 0.6f + rnd() * 0.3f) * decay(t, 9.f);
      });
    case Sfx::PlayerDeath:
      return render(1.4f, [](float t, float d) {
        return (chirp(320.f, 40.f, t, d) * 0.6f + saw(70.f, t) * 0.2f) * decay(t, 2.2f);
      });
    case Sfx::Jump:
      return render(0.16f, [](float t, float d) { return chirp(200.f, 420.f, t, d) * 0.35f * decay(t, 12.f); });
    case Sfx::Land:
      return render(0.14f, [lp = LowPass{}](float t, float) mutable {
        return (lp(rnd(), 0.15f) + sine(55.f, t)) * 0.8f * decay(t, 30.f);
      });
    case Sfx::Pickup:
      return render(0.45f, [](float t, float) {
        const float f = t < 0.1f ? 660.f : (t < 0.2f ? 880.f : 1320.f);
        return sine(f, t) * 0.4f * decay(fmodf(t, 0.1f) + (t > 0.2f ? t - 0.2f : 0.f), 6.f);
      });
    case Sfx::Potion:
      return render(0.6f, [](float t, float d) {
        return sine(500.f + 300.f * sine(9.f, t) + 400.f * t / d, t) * 0.3f * sinf(kPi * t / d);
      });
    case Sfx::KeyUse:
      return render(0.35f, [](float t, float) {
        const float t2 = std::max(0.f, t - 0.15f);
        return (sine(1800.f, t) * decay(t, 40.f) + (t > 0.15f ? sine(1300.f, t2) * decay(t2, 35.f) : 0.f)) * 0.5f;
      });
    case Sfx::DoorOpen:
      return render(1.1f, [lp = LowPass{}](float t, float d) mutable {
        const float creak = saw(70.f + 40.f * t / d + 6.f * sine(7.f, t), t) * 0.25f;
        return (creak + lp(rnd(), 0.05f) * 0.8f) * sinf(kPi * t / d);
      });
    case Sfx::DoorLocked:
      return render(0.25f, [](float t, float) {
        return (sine(90.f, t) * 0.6f + rnd() * 0.3f + sine(1500.f, t) * 0.2f) * decay(t, 22.f);
      });
    case Sfx::StoneDrag:
      return render(0.4f, [lp = LowPass{}](float t, float d) mutable {
        return lp(rnd(), 0.06f) * 2.4f * sinf(kPi * t / d) + sine(45.f, t) * 0.25f;
      });
    case Sfx::PlateClick:
      return render(0.3f, [](float t, float) {
        return (sine(2200.f, t) * 0.3f * decay(t, 60.f)) + sine(70.f, t) * 0.7f * decay(t, 14.f);
      });
    case Sfx::GateOpen:
      return render(1.8f, [lp = LowPass{}](float t, float d) mutable {
        return (lp(rnd(), 0.05f) * 2.0f + sine(40.f + 6.f * sine(3.f, t), t) * 0.5f +
                saw(55.f, t) * 0.15f * (0.5f + 0.5f * sine(11.f, t))) * sinf(kPi * t / d);
      });
    case Sfx::TorchToggle:
      return render(0.35f, [lp = LowPass{}](float t, float d) mutable {
        return lp(rnd(), 0.03f + 0.4f * (1.f - t / d)) * 1.8f * sinf(kPi * t / d);
      });
    case Sfx::EchoStart:
      return render(1.2f, [](float t, float d) {
        const float e = sinf(kPi * t / d);
        return (sine(440.f, t) + sine(443.f, t) + sine(660.f, t) * 0.6f + sine(880.f + 40.f * sine(5.f, t), t) * 0.4f) * 0.18f * e;
      });
    case Sfx::Win:
      return render(2.2f, [](float t, float d) {
        const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
        float s = 0.f;
        for (int i = 0; i < 4; ++i) {
          const float x = t - 0.18f * i;
          if (x > 0.f) s += sine(notes[i], x) * 0.25f * decay(x, 1.6f);
        }
        return s;
      });
    case Sfx::UiClick:
      return render(0.08f, [](float t, float) { return sine(900.f, t) * 0.4f * decay(t, 50.f); });
    default:
      return render(0.01f, [](float, float) { return 0.f; });
  }
}

Wave ambience() {
  // Every frequency fits a whole number of cycles in 8 s, so the loop is seamless.
  return render(8.f, [](float t, float) {
    float s = sine(55.f, t) * 0.30f + sine(82.5f, t) * 0.18f * (0.6f + 0.4f * sine(0.25f, t)) +
              sine(110.f, t) * 0.08f * (0.5f + 0.5f * sine(0.125f, t));
    const float drips[] = {2.3f, 5.6f};
    for (float d0 : drips) {
      const float x = t - d0;
      if (x > 0.f) s += sine(1300.f - 500.f * x, x) * 0.2f * decay(x, 16.f);
    }
    return s;
  });
}

}  // namespace synth
