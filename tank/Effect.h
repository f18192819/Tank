#pragma once

#include "Types.h"

enum class EffectType
{
    Warning,
    Explosion,
    LaserTrace,
    SpawnWarning,
    Decoy,
    AirStrikeWarning,
    Smoke
};

struct TimedEffect
{
    EffectType type;
    Vec2 position;
    int ticks;
    int totalTicks;
    bool fromPlayer;
    char glyph;
    bool explodes;
    DamageType damageType;
    int power;

    TimedEffect()
        : type(EffectType::Warning),
          position(),
          ticks(0),
          totalTicks(0),
          fromPlayer(false),
          glyph('!'),
          explodes(false),
          damageType(DamageType::BombExplosion),
          power(1)
    {
    }

    TimedEffect(const Vec2& pos, int duration, bool playerOwned, char mark, bool willExplode,
        EffectType effectType = EffectType::Warning,
        DamageType damage = DamageType::BombExplosion,
        int damagePower = 1)
        : type(effectType),
          position(pos),
          ticks(duration),
          totalTicks(duration),
          fromPlayer(playerOwned),
          glyph(mark),
          explodes(willExplode),
          damageType(damage),
          power(damagePower)
    {
    }
};
