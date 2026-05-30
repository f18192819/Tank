#pragma once

#include <cmath>

struct Vec2
{
    int x;
    int y;

    Vec2() : x(0), y(0) {}
    Vec2(int xValue, int yValue) : x(xValue), y(yValue) {}

    bool operator==(const Vec2& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vec2& other) const
    {
        return !(*this == other);
    }

    Vec2 operator+(const Vec2& other) const
    {
        return Vec2(x + other.x, y + other.y);
    }

    Vec2 operator-(const Vec2& other) const
    {
        return Vec2(x - other.x, y - other.y);
    }
};

struct FloatVec2
{
    double x;
    double y;

    FloatVec2() : x(0.0), y(0.0) {}
    FloatVec2(double xValue, double yValue) : x(xValue), y(yValue) {}
    explicit FloatVec2(const Vec2& value) : x(static_cast<double>(value.x) + 0.5), y(static_cast<double>(value.y) + 0.5) {}
};

using GridPosition = Vec2;

struct RectI
{
    int left;
    int top;
    int right;
    int bottom;

    RectI() : left(0), top(0), right(0), bottom(0) {}
    RectI(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}

    bool contains(const Vec2& position) const
    {
        return position.x >= left && position.x <= right && position.y >= top && position.y <= bottom;
    }
};

inline Vec2 ToCell(const FloatVec2& position)
{
    return Vec2(static_cast<int>(position.x), static_cast<int>(position.y));
}

inline Vec2 ToNearestCell(const FloatVec2& position)
{
    return ToCell(position);
}

enum class Direction
{
    Up,
    Right,
    Down,
    Left
};

enum class EnemyType
{
    Player,
    BasicEnemyA,
    Scout = BasicEnemyA,
    BasicEnemyB,
    Armor = BasicEnemyB,
    LaserShieldEnemy,
    LaserGuard = LaserShieldEnemy,
    BombThrowEnemy,
    Bomber = BombThrowEnemy,
    SuicideEnemy,
    Kamikaze = SuicideEnemy,
    BossLevel2,
    StageTwoBoss = BossLevel2,
    BossLevel3,
    StageThreeBoss = BossLevel3,
    FinalBoss
};

using EnemyKind = EnemyType;

enum class TerrainType
{
    Empty,
    WoodenBox,
    Crate = WoodenBox,
    Wall,
    Base,
    Trench,
    Swamp,
    NormalSpawnPoint,
    NormalSpawner = NormalSpawnPoint,
    EliteSpawnPoint,
    EliteSpawner = EliteSpawnPoint
};

using Tile = TerrainType;

enum class PowerUpType
{
    Shield,
    BlastShell,
    BombShell = BlastShell,
    Laser,
    Shovel,
    Decoy
};

enum class SkillType
{
    None,
    LaserBurst,
    ShieldPulse,
    BombThrow,
    SuicideCharge,
    Invisibility,
    BombSummon,
    MachineGun,
    SummonMinion,
    BulletReflect,
    AirStrike
};

enum class BulletKind
{
    Normal,
    Blast,
    Reflected
};

enum class DamageType
{
    NormalShell,
    BlastShell,
    Laser,
    BombExplosion,
    SuicideExplosion,
    AirStrike
};

enum class GameState
{
    StageIntro,
    Running,
    StageCleared,
    Victory,
    Defeat,
    Quit
};

enum class Difficulty
{
    Easy,
    Normal,
    Hard
};

struct TankStats
{
    int maxLives;
    int fireCooldownTicks;
    int bulletSpeed;
    int moveInterval;
    double moveSpeed;
    EnemyType type;
    int scoreValue;
    bool boss;

    TankStats()
        : maxLives(1),
          fireCooldownTicks(20),
          bulletSpeed(1),
          moveInterval(4),
          moveSpeed(0.09),
          type(EnemyType::BasicEnemyA),
          scoreValue(100),
          boss(false)
    {
    }
};

struct DamageInfo
{
    bool fromPlayer;
    DamageType type;
    Direction direction;
    BulletKind bulletKind;
    EnemyType sourceType;
    int power;

    DamageInfo()
        : fromPlayer(false),
          type(DamageType::NormalShell),
          direction(Direction::Up),
          bulletKind(BulletKind::Normal),
          sourceType(EnemyType::BasicEnemyA),
          power(1)
    {
    }
};

inline Vec2 DirectionDelta(Direction direction)
{
    switch (direction)
    {
    case Direction::Up:
        return Vec2(0, -1);
    case Direction::Right:
        return Vec2(1, 0);
    case Direction::Down:
        return Vec2(0, 1);
    case Direction::Left:
        return Vec2(-1, 0);
    default:
        return Vec2();
    }
}

inline FloatVec2 DirectionFloatDelta(Direction direction)
{
    Vec2 delta = DirectionDelta(direction);
    return FloatVec2(static_cast<double>(delta.x), static_cast<double>(delta.y));
}

inline char DirectionGlyph(Direction direction)
{
    switch (direction)
    {
    case Direction::Up:
        return '^';
    case Direction::Right:
        return '>';
    case Direction::Down:
        return 'v';
    case Direction::Left:
        return '<';
    default:
        return '?';
    }
}

inline Direction OppositeDirection(Direction direction)
{
    switch (direction)
    {
    case Direction::Up:
        return Direction::Down;
    case Direction::Right:
        return Direction::Left;
    case Direction::Down:
        return Direction::Up;
    case Direction::Left:
        return Direction::Right;
    default:
        return Direction::Up;
    }
}

inline int ManhattanDistance(const Vec2& a, const Vec2& b)
{
    const int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    const int dy = a.y > b.y ? a.y - b.y : b.y - a.y;
    return dx + dy;
}

inline int ClampInt(int value, int minValue, int maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }

    if (value > maxValue)
    {
        return maxValue;
    }

    return value;
}

inline bool IsAligned(const Vec2& a, const Vec2& b)
{
    return a.x == b.x || a.y == b.y;
}

inline Direction DirectionToward(const Vec2& from, const Vec2& to)
{
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    if (std::abs(dx) > std::abs(dy))
    {
        return dx >= 0 ? Direction::Right : Direction::Left;
    }

    return dy >= 0 ? Direction::Down : Direction::Up;
}
