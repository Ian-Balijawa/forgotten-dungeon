#pragma once
#include <raylib.h>

#include "world/World.hpp"

namespace hud
{
    void menu();
    void playing(const World &world, bool firstPerson);
    void overlay(const char *title, const char *subtitle, Color color);
} // namespace hud
