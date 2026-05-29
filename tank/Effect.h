#pragma once

#include "Types.h"

struct TimedEffect
{
    Vec2 position;
    int ticks;
    bool fromPlayer;
    char glyph;
    bool explodes;

    TimedEffect(const Vec2& pos, int duration, bool playerOwned, char mark, bool willExplode)
        : position(pos), ticks(duration), fromPlayer(playerOwned), glyph(mark), explodes(willExplode)
    {
    }
};
