#include "Entity.h"

Entity::Entity(const Vec2& position)
    : position_(position), precisePosition_(position), alive_(true)
{
}

const Vec2& Entity::position() const
{
    return position_;
}

const FloatVec2& Entity::precisePosition() const
{
    return precisePosition_;
}

bool Entity::isAlive() const
{
    return alive_;
}

void Entity::destroy()
{
    alive_ = false;
}

void Entity::setPrecisePosition(const FloatVec2& position)
{
    precisePosition_ = position;
    position_ = ToNearestCell(position);
}
