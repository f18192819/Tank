#pragma once

#include "Entity.h"

class Bullet : public Entity
{
public:
    Bullet(const Vec2& position, Direction direction, bool fromPlayer, int speed);
    Bullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed);
    Bullet(const Vec2& position, Direction direction, bool fromPlayer, int speed, BulletKind kind);
    Bullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed, BulletKind kind);

    bool fromPlayer() const;
    Direction direction() const;
    BulletKind kind() const;
    int speed() const;
    const Vec2& previousPosition() const;
    char glyph() const override;
    void update(Game& game) override;

private:
    Vec2 previousPosition_;
    Direction direction_;
    bool fromPlayer_;
    bool justSpawned_;
    int speed_;
    BulletKind kind_;
};
