#include "GameMap.h"

#include <algorithm>
#include <cstdlib>

GameMap::GameMap()
{
    reset();
}

void GameMap::reset()
{
    static const char* layout[Height] =
    {
        "#########################################",
        "#       %     ~    N    ~     %      E#",
        "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
        "#   %   T   %   ~   %   ~   %   T   % #",
        "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
        "#   ~   %           #           %   ~  #",
        "# %%% %%% %%%%%%%   #   %%%%%%% %%% % #",
        "#   %   T   %       #       %   T   % #",
        "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
        "#   ~   %           %           %   ~  #",
        "##### ##### ##### ##### ##### ##### ####",
        "#       T        ~     ~        T       #",
        "# #### ##### ##### ##### ##### ##### ###",
        "#      %           %           %       #",
        "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
        "#  %       %       #       %       %   #",
        "# %%% %%% %%%%%%%  #  %%%%%%% %%% %%% #",
        "#   ~  %     X     #     X     %   ~   #",
        "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
        "#  %       %       %       %       %   #",
        "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
        "#       %     %    BBB    %     %      #",
        "#########################################"
    };

    tiles_.clear();
    for (int y = 0; y < Height; ++y)
    {
        std::string row(layout[y]);
        if (static_cast<int>(row.size()) < Width)
        {
            const std::size_t insertAt = row.empty() ? 0 : row.size() - 1;
            row.insert(insertAt, Width - row.size(), ' ');
        }
        else if (static_cast<int>(row.size()) > Width)
        {
            row.resize(Width);
        }

        tiles_.push_back(row);
    }
    baseDestroyed_ = false;
}

bool GameMap::inBounds(const Vec2& position) const
{
    return position.x >= 0 && position.x < Width && position.y >= 0 && position.y < Height;
}

Tile GameMap::tileAt(const Vec2& position) const
{
    if (!inBounds(position))
    {
        return Tile::Wall;
    }

    return decode(tiles_[position.y][position.x]);
}

bool GameMap::isPassable(const Vec2& position) const
{
    const Tile tile = tileAt(position);
    return tile == Tile::Empty || tile == Tile::Trench || tile == Tile::Swamp ||
        tile == Tile::NormalSpawner || tile == Tile::EliteSpawner;
}

bool GameMap::slowsMovement(const Vec2& position) const
{
    return tileAt(position) == Tile::Swamp;
}

bool GameMap::isSpawner(const Vec2& position) const
{
    const Tile tile = tileAt(position);
    return tile == Tile::NormalSpawner || tile == Tile::EliteSpawner;
}

bool GameMap::canBecomeTrench(const Vec2& position) const
{
    const Tile tile = tileAt(position);
    return tile == Tile::Empty || tile == Tile::Swamp || tile == Tile::Crate || tile == Tile::Trench;
}

bool GameMap::isBulletBlocked(const Vec2& position) const
{
    const Tile tile = tileAt(position);
    return tile == Tile::Crate || tile == Tile::Wall || tile == Tile::Base;
}

bool GameMap::damageTile(const Vec2& position)
{
    if (!inBounds(position))
    {
        return false;
    }

    const Tile tile = tileAt(position);
    if (tile == Tile::Crate)
    {
        tiles_[position.y][position.x] = encode(Tile::Empty);
        return true;
    }

    if (tile == Tile::Base)
    {
        baseDestroyed_ = true;
        tiles_[position.y][position.x] = 'X';
        return true;
    }

    return tile == Tile::Wall;
}

bool GameMap::setTrench(const Vec2& position)
{
    if (!inBounds(position) || isSpawner(position) || tileAt(position) == Tile::Wall || tileAt(position) == Tile::Base)
    {
        return false;
    }

    tiles_[position.y][position.x] = encode(Tile::Trench);
    return true;
}

bool GameMap::isBaseDestroyed() const
{
    return baseDestroyed_;
}

char GameMap::glyphAt(const Vec2& position) const
{
    if (!inBounds(position))
    {
        return '#';
    }

    return tiles_[position.y][position.x];
}

std::vector<Vec2> GameMap::normalSpawners() const
{
    std::vector<Vec2> spawners;
    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            if (decode(tiles_[y][x]) == Tile::NormalSpawner)
            {
                spawners.push_back(Vec2(x, y));
            }
        }
    }

    return spawners;
}

std::vector<Vec2> GameMap::eliteSpawners() const
{
    std::vector<Vec2> spawners;
    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            if (decode(tiles_[y][x]) == Tile::EliteSpawner)
            {
                spawners.push_back(Vec2(x, y));
            }
        }
    }

    return spawners;
}

Vec2 GameMap::randomOpenCell() const
{
    for (int attempts = 0; attempts < 400; ++attempts)
    {
        const Vec2 position(1 + std::rand() % (Width - 2), 1 + std::rand() % (Height - 2));
        if (isPassable(position) && !isSpawner(position))
        {
            return position;
        }
    }

    return Vec2(1, 1);
}

void GameMap::draw(std::vector<std::string>& buffer) const
{
    for (int y = 0; y < Height && y < static_cast<int>(buffer.size()); ++y)
    {
        for (int x = 0; x < Width && x < static_cast<int>(buffer[y].size()); ++x)
        {
            const char glyph = tiles_[y][x];
            buffer[y][x] = glyph == 'E' ? ' ' : glyph;
        }
    }
}

Tile GameMap::decode(char glyph) const
{
    switch (glyph)
    {
    case '%':
        return Tile::Crate;
    case '#':
        return Tile::Wall;
    case 'B':
    case 'X':
        return Tile::Base;
    case 'T':
        return Tile::Trench;
    case '~':
        return Tile::Swamp;
    case 'N':
        return Tile::NormalSpawner;
    case 'E':
        return Tile::EliteSpawner;
    default:
        return Tile::Empty;
    }
}

char GameMap::encode(Tile tile) const
{
    switch (tile)
    {
    case Tile::Crate:
        return '%';
    case Tile::Wall:
        return '#';
    case Tile::Base:
        return 'B';
    case Tile::Trench:
        return 'T';
    case Tile::Swamp:
        return '~';
    case Tile::NormalSpawner:
        return 'N';
    case Tile::EliteSpawner:
        return 'E';
    case Tile::Empty:
    default:
        return ' ';
    }
}
