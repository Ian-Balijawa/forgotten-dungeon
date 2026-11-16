#pragma once
#include <raylib.h>
#include "audio/AudioManager.hpp"

namespace synth
{
    Wave generate(Sfx id); // one-shot effect
    Wave ambience();       // seamless 8 s dungeon drone
    float defaultVolume(Sfx id);
} // namespace synth
