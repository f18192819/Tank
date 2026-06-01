#include "Bullet.h"

#include "Game.h"

Bullet::Bullet(const Vec2& position, Direction direction, bool fromPlayer, int speed)
    : Bullet(position, direction, fromPlayer, speed, BulletKind::Normal)
{
}

Bullet::Bullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed)
    : Bullet(position, direction, fromPlayer, speed, BulletKind::Normal)
{
}

Bullet::Bullet(const Vec2& position, Direction direction, bool fromPlayer, int speed, BulletKind kind)
    : Entity(position),
      previousPosition_(position),
      direction_(direction),
      fromPlayer_(fromPlayer),
      justSpawned_(true),
      speed_(speed),
      kind_(kind)
{
}

Bullet::Bullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed, BulletKind kind)
    : Entity(ToCell(position)),
      previousPosition_(ToCell(position)),
      direction_(direction),
      fromPlayer_(fromPlayer),
      justSpawned_(true),
      speed_(speed),
      kind_(kind)
{
    setPrecisePosition(position);
    previousPosition_ = position_;
}

bool Bullet::fromPlayer() const
{
    return fromPlayer_;
}

Direction Bullet::direction() const
{
    return direction_;
}

BulletKind Bullet::kind() const
{
    return kind_;
}

int Bullet::speed() const
{
    return speed_;
}

const Vec2& Bullet::previousPosition() const
{
    return previousPosition_;
}

char Bullet::glyph() const
{
    if (kind_ == BulletKind::Blast)
    {
        return 'o';
    }

    return '*';
}

void Bullet::update(Game& game)
{
    if (justSpawned_)
    {
        justSpawned_ = false;
        return;
    }

    previousPosition_ = position_;
    const double stepSize = 0.16 * static_cast<double>(speed_);
    const int subSteps = 3;
    const FloatVec2 delta = DirectionFloatDelta(direction_);
    for (int step = 0; step < subSteps && isAlive(); ++step)
    {
        const FloatVec2 nextPrecise(
            precisePosition_.x + delta.x * stepSize / subSteps,
            precisePosition_.y + delta.y * stepSize / subSteps);
        if (game.resolveBulletHit(*this, nextPrecise))
        {
            destroy();
            return;
        }

        setPrecisePosition(nextPrecise);
    }
}
