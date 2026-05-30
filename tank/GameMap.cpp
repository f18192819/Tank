#include "GameMap.h"

#include <algorithm>
#include <cstdlib>
#include <queue>

namespace
{
    const char* const kLevelLayouts[4][GameMap::Height] =
    {
        {
            "#########################################",
            "#  N    %    ~      %      ~    %    N  #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%%  #",
            "#   %   T   %   ~   %   ~   %   T   %  #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   ~   %           #           %   ~  #",
            "# %%% %%% %%%%%%%   #   %%%%%%% %%% % #",
            "#   %   T   %       #       %   T   %  #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   ~   %           %           %   ~  #",
            "##### ##### ##### ##### ##### ##### ####",
            "#       T        ~     ~        T      #",
            "# #### ##### ##### ##### ##### ##### ###",
            "#      %           %           %       #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#  %       %       #       %       %   #",
            "# %%% %%% %%%%%%%  #  %%%%%%% %%% %%% #",
            "#   ~  %     X     #     X     %   ~ E #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#  %       %       %       %       %   #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#      %      %    BBB    %      %     #",
            "#########################################"
        },
        {
            "#########################################",
            "# N   %%%    ~     EEE     ~    %%%   N#",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   T   %       ~     ~       %   T    #",
            "# %%% %%% %%% %%% ### %%% %%% %%% %%% #",
            "#   ~       %       #       %      ~   #",
            "# %%% %%%%%%% %%%   #   %%% %%%%%%% %% #",
            "#   %   T       %   #   %       T   %  #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#      %       %         %       %     #",
            "###### ##### ##### ### ##### ##### #####",
            "#      T      ~     ~     ~      T     #",
            "# #### ##### ##### ### ##### ##### #### #",
            "#  %       %           %       %     E #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   ~   %       #   #       %   ~      #",
            "# %%% %%% %%%%% #   # %%%%% %%% %%% % #",
            "#   %      X    #   #    X      %      #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#      %       %   ~   %       %      #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   N      %    %  BBB  %    %      N #",
            "#########################################"
        },
        {
            "#########################################",
            "# E   %%%   ~    N    ~    %%%   E    N#",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   T    %    ~    #    ~    %    T    #",
            "# %%% %%% %%% %%% ### %%% %%% %%% %%% #",
            "#   ~       %      ###      %      ~   #",
            "# %%% %%%%%%% %%%  ###  %%% %%%%%%% %% #",
            "#   %   T       %   #   %       T   %  #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#      %     ~   %     %   ~     %    #",
            "###### ##### ### ##### ##### ### ##### #",
            "#   T    ~      %  E  %      ~    T   #",
            "# #### ##### ### ##### ### ##### #### #",
            "#  %      X   %   ###   %   X      %  #",
            "# %%% %%% %%% %%% ### %%% %%% %%% %%% #",
            "#   ~   %       #  #  #       %   ~    #",
            "# %%% %%% %%%%% #  #  # %%%%% %%% %%% #",
            "#   %       E   #     #   E       %    #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#      %     ~   %     %   ~     %    #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   N   %    %    BBB    %    %   N   #",
            "#########################################"
        },
        {
            "#########################################",
            "# E   %%%   ~    N    ~    %%%   E   N #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   T    %    ~   ###   ~    %    T    #",
            "# %%% %%% %%% %%% ### %%% %%% %%% %%% #",
            "#   ~    E  %      ###      %  E  ~    #",
            "# %%% %%%%%%% %%%  ###  %%% %%%%%%% %% #",
            "#   %   T       %   #   %       T   %  #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   N  %     ~   %  E  %   ~     %  N #",
            "###### ##### ### ##### ##### ### ##### #",
            "#   T    ~      %  E  %      ~    T   #",
            "# #### ##### ### ##### ### ##### #### #",
            "#  %      X   %   ###   %   X      %  #",
            "# %%% %%% %%% %%% ### %%% %%% %%% %%% #",
            "#   ~   %   E   #  #  #   E   %   ~    #",
            "# %%% %%% %%%%% #  #  # %%%%% %%% %%% #",
            "#   %      E    #     #    E      %    #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   N  %     ~   %     %   ~     %  N #",
            "# %%% %%% %%% %%% %%% %%% %%% %%% %%% #",
            "#   N   %    %    BBB    %    %   E   #",
            "#########################################"
        }
    };
}

GameMap::GameMap()
    : baseDestroyed_(false),
      currentLevel_(1)
{
    reset();
}

void GameMap::reset()
{
    loadLevel(1);
}

void GameMap::loadLevel(int level)
{
    currentLevel_ = ClampInt(level, 1, 4);
    tiles_.clear();
    for (int y = 0; y < Height; ++y)
    {
        std::string row(kLevelLayouts[currentLevel_ - 1][y]);
        if (static_cast<int>(row.size()) < Width)
        {
            row.insert(row.size(), Width - static_cast<int>(row.size()), ' ');
        }
        else if (static_cast<int>(row.size()) > Width)
        {
            row.resize(Width);
        }

        tiles_.push_back(row);
    }
    baseDestroyed_ = false;
}

int GameMap::level() const
{
    return currentLevel_;
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
        tile == Tile::NormalSpawnPoint || tile == Tile::EliteSpawnPoint;
}

bool GameMap::slowsMovement(const Vec2& position) const
{
    return tileAt(position) == Tile::Swamp;
}

bool GameMap::isSpawner(const Vec2& position) const
{
    const Tile tile = tileAt(position);
    return tile == Tile::NormalSpawnPoint || tile == Tile::EliteSpawnPoint;
}

bool GameMap::canBecomeTrench(const Vec2& position) const
{
    const Tile tile = tileAt(position);
    return tile == Tile::Empty || tile == Tile::Swamp || tile == Tile::Trench;
}

bool GameMap::isBulletBlocked(const Vec2& position) const
{
    const Tile tile = tileAt(position);
    return tile == Tile::WoodenBox || tile == Tile::Wall || tile == Tile::Base;
}

bool GameMap::damageTile(const Vec2& position, bool fromPlayer, DamageType damageType)
{
    if (!inBounds(position))
    {
        return false;
    }

    const Tile tile = tileAt(position);
    if (tile == Tile::WoodenBox)
    {
        if (fromPlayer || damageType != DamageType::NormalShell)
        {
            tiles_[position.y][position.x] = encode(Tile::Empty);
            return true;
        }
        return false;
    }

    if (tile == Tile::Base)
    {
        if (!fromPlayer)
        {
            baseDestroyed_ = true;
            tiles_[position.y][position.x] = 'X';
            return true;
        }
        return false;
    }

    return tile == Tile::Wall;
}

bool GameMap::setTrench(const Vec2& position)
{
    if (!inBounds(position) || !canBecomeTrench(position) || isSpawner(position))
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
            if (decode(tiles_[y][x]) == Tile::NormalSpawnPoint)
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
            if (decode(tiles_[y][x]) == Tile::EliteSpawnPoint)
            {
                spawners.push_back(Vec2(x, y));
            }
        }
    }
    return spawners;
}

Vec2 GameMap::randomOpenCell() const
{
    for (int attempts = 0; attempts < 500; ++attempts)
    {
        const Vec2 position(1 + std::rand() % (Width - 2), 1 + std::rand() % (Height - 2));
        if (isPassable(position) && !isSpawner(position))
        {
            return position;
        }
    }

    return Vec2(1, 1);
}

bool GameMap::hasLineOfSightOrthogonal(const Vec2& from, const Vec2& to) const
{
    if (!IsAligned(from, to))
    {
        return false;
    }

    if (from.x == to.x)
    {
        const int minY = std::min(from.y, to.y) + 1;
        const int maxY = std::max(from.y, to.y);
        for (int y = minY; y < maxY; ++y)
        {
            if (tileAt(Vec2(from.x, y)) == Tile::Wall)
            {
                return false;
            }
        }
        return true;
    }

    const int minX = std::min(from.x, to.x) + 1;
    const int maxX = std::max(from.x, to.x);
    for (int x = minX; x < maxX; ++x)
    {
        if (tileAt(Vec2(x, from.y)) == Tile::Wall)
        {
            return false;
        }
    }
    return true;
}

Vec2 GameMap::findBombLanding(const Vec2& from, const Vec2& toward, int maxRange) const
{
    const Direction direction = DirectionToward(from, toward);
    Vec2 last = from;
    Vec2 current = from;
    for (int step = 0; step < maxRange; ++step)
    {
        current = current + DirectionDelta(direction);
        if (!inBounds(current))
        {
            break;
        }

        const Tile tile = tileAt(current);
        if (tile == Tile::Wall || tile == Tile::Base)
        {
            break;
        }

        last = current;
        if (current == toward)
        {
            break;
        }
    }

    return last;
}

std::vector<Vec2> GameMap::findPath(const Vec2& start, const Vec2& goal) const
{
    std::vector<Vec2> empty;
    if (!inBounds(start) || !inBounds(goal))
    {
        return empty;
    }

    if (start == goal)
    {
        empty.push_back(start);
        return empty;
    }

    const int total = Width * Height;
    std::vector<int> parent(total, -1);
    std::queue<int> queue;
    const int startIndex = start.y * Width + start.x;
    const int goalIndex = goal.y * Width + goal.x;

    parent[startIndex] = startIndex;
    queue.push(startIndex);

    while (!queue.empty() && parent[goalIndex] == -1)
    {
        const int index = queue.front();
        queue.pop();
        const Vec2 current(index % Width, index / Width);
        const Vec2 neighbors[4] =
        {
            current + DirectionDelta(Direction::Up),
            current + DirectionDelta(Direction::Right),
            current + DirectionDelta(Direction::Down),
            current + DirectionDelta(Direction::Left)
        };

        for (int i = 0; i < 4; ++i)
        {
            const Vec2 next = neighbors[i];
            if (!inBounds(next))
            {
                continue;
            }

            const int nextIndex = next.y * Width + next.x;
            if (parent[nextIndex] != -1)
            {
                continue;
            }

            if (!isPassable(next) && next != goal)
            {
                continue;
            }

            parent[nextIndex] = index;
            queue.push(nextIndex);
        }
    }

    if (parent[goalIndex] == -1)
    {
        return empty;
    }

    std::vector<Vec2> path;
    for (int index = goalIndex; index != startIndex; index = parent[index])
    {
        path.push_back(Vec2(index % Width, index / Width));
    }
    std::reverse(path.begin(), path.end());
    return path;
}

void GameMap::draw(std::vector<std::string>& buffer) const
{
    for (int y = 0; y < Height && y < static_cast<int>(buffer.size()); ++y)
    {
        for (int x = 0; x < Width && x < static_cast<int>(buffer[y].size()); ++x)
        {
            buffer[y][x] = tiles_[y][x];
        }
    }
}

Tile GameMap::decode(char glyph) const
{
    switch (glyph)
    {
    case '%':
        return Tile::WoodenBox;
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
        return Tile::NormalSpawnPoint;
    case 'E':
        return Tile::EliteSpawnPoint;
    default:
        return Tile::Empty;
    }
}

char GameMap::encode(Tile tile) const
{
    switch (tile)
    {
    case Tile::WoodenBox:
        return '%';
    case Tile::Wall:
        return '#';
    case Tile::Base:
        return 'B';
    case Tile::Trench:
        return 'T';
    case Tile::Swamp:
        return '~';
    case Tile::NormalSpawnPoint:
        return 'N';
    case Tile::EliteSpawnPoint:
        return 'E';
    case Tile::Empty:
    default:
        return ' ';
    }
}
