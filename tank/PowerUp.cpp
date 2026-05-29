#include "PowerUp.h"

PowerUp::PowerUp(const Vec2& position, PowerUpType type)
    : Entity(position), type_(type)
{
}

PowerUpType PowerUp::type() const
{
    return type_;
}

char PowerUp::glyph() const
{
    switch (type_)
    {
    case PowerUpType::Shield:
        return 'S';
    case PowerUpType::BombShell:
        return 'F';
    case PowerUpType::Laser:
        return 'R';
    case PowerUpType::Shovel:
        return 'Q';
    case PowerUpType::Decoy:
        return 'H';
    default:
        return '?';
    }
}

void PowerUp::update(Game& game)
{
    (void)game;
}
