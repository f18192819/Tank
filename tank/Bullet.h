#pragma once

#include "Entity.h"

class Bullet : public Entity
{
public:
    Bullet(const Vec2& position, Direction direction, bool fromPlayer, int speed);
    Bullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed);

    bool fromPlayer() const;
    Direction direction() const;
    int speed() const;
    char glyph() const override;
    void update(Game& game) override;

private:
    Direction direction_;
    bool fromPlayer_;
    int speed_;
};
