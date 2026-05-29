#pragma once

#include <string>
#include <vector>

#include "Types.h"

enum class Tile
{
    Empty,
    Crate,
    Wall,
    Base,
    Trench,
    Swamp,
    NormalSpawner,
    EliteSpawner
};

class GameMap
{
public:
    static const int Width = 41;
    static const int Height = 23;

    GameMap();

    void reset();
    bool inBounds(const Vec2& position) const;
    Tile tileAt(const Vec2& position) const;
    bool isPassable(const Vec2& position) const;
    bool slowsMovement(const Vec2& position) const;
    bool isSpawner(const Vec2& position) const;
    bool canBecomeTrench(const Vec2& position) const;
    bool isBulletBlocked(const Vec2& position) const;
    bool damageTile(const Vec2& position);
    bool setTrench(const Vec2& position);
    bool isBaseDestroyed() const;
    char glyphAt(const Vec2& position) const;
    std::vector<Vec2> normalSpawners() const;
    std::vector<Vec2> eliteSpawners() const;
    Vec2 randomOpenCell() const;
    void draw(std::vector<std::string>& buffer) const;

private:
    std::vector<std::string> tiles_;
    bool baseDestroyed_;

    Tile decode(char glyph) const;
    char encode(Tile tile) const;
};
