#pragma once

#include "Types.h"

class Game;

class Entity
{
public:
    explicit Entity(const Vec2& position);
    virtual ~Entity() {}

    const Vec2& position() const;
    const FloatVec2& precisePosition() const;
    bool isAlive() const;
    void destroy();

    virtual char glyph() const = 0;
    virtual void update(Game& game) = 0;

protected:
    Vec2 position_;
    FloatVec2 precisePosition_;
    bool alive_;

    void setPrecisePosition(const FloatVec2& position);
};
