#pragma once

#include <string>
#include <vector>

#include "Types.h"

class GameMap
{
public:
    static const int Width = 41;
    static const int Height = 23;

    GameMap();

    void reset();
    void loadLevel(int level);
    int level() const;
    bool inBounds(const Vec2& position) const;
    Tile tileAt(const Vec2& position) const;
    bool isPassable(const Vec2& position) const;
    bool slowsMovement(const Vec2& position) const;
    bool isSpawner(const Vec2& position) const;
    bool canBecomeTrench(const Vec2& position) const;
    bool isBulletBlocked(const Vec2& position) const;
    bool damageTile(const Vec2& position, bool fromPlayer = true, DamageType damageType = DamageType::NormalShell);
    bool setTrench(const Vec2& position);
    bool isBaseDestroyed() const;
    char glyphAt(const Vec2& position) const;
    std::vector<Vec2> normalSpawners() const;
    std::vector<Vec2> eliteSpawners() const;
    Vec2 randomOpenCell() const;
    bool hasLineOfSightOrthogonal(const Vec2& from, const Vec2& to) const;
    Vec2 findBombLanding(const Vec2& from, const Vec2& toward, int maxRange) const;
    std::vector<Vec2> findPath(const Vec2& start, const Vec2& goal) const;
    void draw(std::vector<std::string>& buffer) const;
    int version() const;

private:
    std::vector<std::string> tiles_;
    bool baseDestroyed_;
    int currentLevel_;
    int version_;

    Tile decode(char glyph) const;
    char encode(Tile tile) const;
};
