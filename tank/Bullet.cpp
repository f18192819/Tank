#include "Bullet.h"

#include "Game.h"

Bullet::Bullet(const Vec2& position, Direction direction, bool fromPlayer, int speed)
    : Entity(position), direction_(direction), fromPlayer_(fromPlayer), speed_(speed)
{
}

Bullet::Bullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed)
    : Entity(ToCell(position)), direction_(direction), fromPlayer_(fromPlayer), speed_(speed)
{
    setPrecisePosition(position);
}

bool Bullet::fromPlayer() const
{
    return fromPlayer_;
}

Direction Bullet::direction() const
{
    return direction_;
}

int Bullet::speed() const
{
    return speed_;
}

char Bullet::glyph() const
{
    return '*';
}

void Bullet::update(Game& game)
{
    const double stepSize = 0.16 * static_cast<double>(speed_);
    const int subSteps = 3;
    const FloatVec2 delta = DirectionFloatDelta(direction_);
    for (int step = 0; step < subSteps && isAlive(); ++step)
    {
        const FloatVec2 nextPrecise(
            precisePosition_.x + delta.x * stepSize / subSteps,
            precisePosition_.y + delta.y * stepSize / subSteps);
        const Vec2 next = ToNearestCell(nextPrecise);
        if (game.resolveBulletHit(*this, next))
        {
            destroy();
            return;
        }

        setPrecisePosition(nextPrecise);
    }
}
