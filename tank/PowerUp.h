#pragma once

#include "Entity.h"

class PowerUp : public Entity
{
public:
    PowerUp(const Vec2& position, PowerUpType type);

    PowerUpType type() const;
    char glyph() const override;
    void update(Game& game) override;

private:
    PowerUpType type_;
};
