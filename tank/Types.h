#pragma once

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
};

struct FloatVec2
{
    double x;
    double y;

    FloatVec2() : x(0.0), y(0.0) {}
    FloatVec2(double xValue, double yValue) : x(xValue), y(yValue) {}
    explicit FloatVec2(const Vec2& value) : x(static_cast<double>(value.x) + 0.5), y(static_cast<double>(value.y) + 0.5) {}
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

enum class EnemyKind
{
    Scout,
    Armor,
    LaserGuard,
    Bomber,
    Kamikaze,
    StageTwoBoss,
    StageThreeBoss,
    FinalBoss
};

enum class PowerUpType
{
    Shield,
    BombShell,
    Laser,
    Shovel,
    Decoy
};

enum class Difficulty
{
    Easy,
    Normal,
    Hard
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
