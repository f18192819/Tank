#include "Game.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <set>
#include <sstream>
#include <ctime>
#include <thread>
#include <utility>
#include <windows.h>

#include "Renderer.h"

Game::Game()
    : player_(Vec2(GameMap::Width / 2, GameMap::Height - 4)),
      state_(GameState::Running),
      score_(0),
      wave_(1),
      spawnTick_(0),
      powerUpTick_(0),
      playerMoveSkip_(0),
      decoyTicks_(0),
      decoyPosition_(Vec2()),
      difficulty_(Difficulty::Normal),
      frameAccumulator_(0),
      playerInTrench_(false)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    reset();
}

void Game::run()
{
    Renderer renderer;
    renderer.showTitle();

    while (state_ != GameState::Quit)
    {
        handleGlobalInput();
        if (state_ == GameState::Running)
        {
            update();
        }

        renderer.draw(*this);
        std::this_thread::sleep_for(std::chrono::milliseconds(70));
    }
}

void Game::tick()
{
    if (state_ == GameState::Running)
    {
        update();
    }
}

void Game::reset()
{
    map_.loadLevel(1);
    player_ = PlayerTank(Vec2(GameMap::Width / 2, GameMap::Height - 4));
    enemies_.clear();
    bullets_.clear();
    powerUps_.clear();
    effects_.clear();
    pendingSpawns_.clear();
    state_ = GameState::Running;
    score_ = 0;
    wave_ = 1;
    spawnTick_ = 0;
    powerUpTick_ = 0;
    playerMoveSkip_ = 0;
    decoyTicks_ = 0;
    frameAccumulator_ = 0;
    playerInTrench_ = false;
    spawnWave();
}

void Game::advanceStage()
{
    if (state_ != GameState::StageCleared)
    {
        return;
    }

    ++wave_;
    if (wave_ > 4)
    {
        state_ = GameState::Victory;
        return;
    }

    spawnTick_ = 0;
    powerUpTick_ = 0;
    playerMoveSkip_ = 0;
    playerInTrench_ = false;
    spawnWave();
    state_ = GameState::Running;
}

void Game::setDifficulty(Difficulty difficulty)
{
    difficulty_ = difficulty;
}

Difficulty Game::difficulty() const
{
    return difficulty_;
}

bool Game::canOccupy(const Vec2& position, const Entity* requester) const
{
    return map_.isPassable(position) && !isOccupiedByTank(position, requester);
}

bool Game::canOccupyPrecise(const FloatVec2& position, const Entity* requester) const
{
    const double radius = 0.22;
    const Vec2 samples[4] =
    {
        ToCell(FloatVec2(position.x - radius, position.y - radius)),
        ToCell(FloatVec2(position.x + radius, position.y - radius)),
        ToCell(FloatVec2(position.x - radius, position.y + radius)),
        ToCell(FloatVec2(position.x + radius, position.y + radius))
    };

    for (int i = 0; i < 4; ++i)
    {
        if (!map_.isPassable(samples[i]))
        {
            return false;
        }
    }

    return !isOccupiedByTank(ToCell(position), requester);
}

bool Game::spawnBullet(const Vec2& position, Direction direction, bool fromPlayer, int speed)
{
    if (!map_.inBounds(position))
    {
        return false;
    }

    if (map_.isBulletBlocked(position))
    {
        map_.damageTile(position, fromPlayer, DamageType::NormalShell);
        return true;
    }

    bullets_.push_back(std::unique_ptr<Bullet>(new Bullet(position, direction, fromPlayer, speed)));
    return true;
}

bool Game::spawnBullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed)
{
    const Vec2 cell = ToCell(position);
    if (!map_.inBounds(cell))
    {
        return false;
    }

    if (map_.isBulletBlocked(cell))
    {
        if (fromPlayer || map_.tileAt(cell) != Tile::Crate)
        {
            map_.damageTile(cell, fromPlayer, DamageType::NormalShell);
        }

        return true;
    }

    bullets_.push_back(std::unique_ptr<Bullet>(new Bullet(position, direction, fromPlayer, speed)));
    return true;
}

bool Game::resolveBulletHit(const Bullet& bullet, const Vec2& target)
{
    if (!map_.inBounds(target))
    {
        return true;
    }

    if (map_.isBulletBlocked(target))
    {
        if (bullet.fromPlayer() || map_.tileAt(target) != Tile::Crate)
        {
            map_.damageTile(target, bullet.fromPlayer(), DamageType::NormalShell);
        }

        return true;
    }

    if (!bullet.fromPlayer() && playerInTrench_)
    {
        return false;
    }

    damageTankAt(target, bullet.fromPlayer(), DamageType::NormalShell, bullet.direction());

    for (std::size_t i = 0; i < bullets_.size(); ++i)
    {
        Bullet* other = bullets_[i].get();
        if (&bullet != other && other->isAlive() && other->position() == target)
        {
            other->destroy();
            return true;
        }
    }

    return !map_.isPassable(target) || isOccupiedByTank(target, nullptr);
}

bool Game::buildTrench(const Vec2& position)
{
    return map_.setTrench(position);
}

bool Game::isSlowed(const Vec2& position) const
{
    return map_.slowsMovement(position);
}

bool Game::tryEnterTrench()
{
    if (map_.tileAt(player_.position()) != Tile::Trench)
    {
        playerInTrench_ = false;
        return false;
    }

    playerInTrench_ = !playerInTrench_;
    return playerInTrench_;
}

void Game::explodeArea(const Vec2& center, bool fromPlayer)
{
    for (int y = center.y - 1; y <= center.y + 1; ++y)
    {
        for (int x = center.x - 1; x <= center.x + 1; ++x)
        {
            const Vec2 position(x, y);
            if (map_.inBounds(position))
            {
                map_.damageTile(position, fromPlayer, DamageType::BombExplosion);
                damageTankAt(position, fromPlayer, DamageType::BombExplosion, Direction::Up);
            }
        }
    }
}

void Game::fireLaser(const Vec2& start, Direction direction, bool fromPlayer)
{
    Vec2 position = start + DirectionDelta(direction);
    while (map_.inBounds(position))
    {
        effects_.push_back(TimedEffect(
            position,
            5,
            fromPlayer,
            (direction == Direction::Up || direction == Direction::Down) ? '|' : '-',
            false,
            EffectType::LaserTrace,
            DamageType::Laser));
        damageTankAt(position, fromPlayer, DamageType::Laser, direction);
        position = position + DirectionDelta(direction);
    }
}

void Game::markDanger(const Vec2& center, int ticks, bool fromPlayer)
{
    effects_.push_back(TimedEffect(center, ticks, fromPlayer, '!', true, EffectType::Warning, DamageType::BombExplosion));
}

void Game::throwBomb(const Vec2& from, const Vec2& toward, bool fromPlayer, int range, int warningTicks)
{
    const Vec2 landing = map_.findBombLanding(from, toward, range);
    effects_.push_back(TimedEffect(landing, warningTicks, fromPlayer, '!', true, EffectType::Warning, DamageType::BombExplosion));
}

void Game::placeDecoy(const Vec2& position)
{
    if (!map_.isPassable(position))
    {
        return;
    }

    decoyPosition_ = position;
    decoyTicks_ = 15 * 14;
    effects_.push_back(TimedEffect(position, decoyTicks_, true, 'H', false, EffectType::Decoy, DamageType::NormalShell));
}

void Game::machineGun(const Vec2& center)
{
    spawnBullet(center + DirectionDelta(Direction::Up), Direction::Up, false, 2);
    spawnBullet(center + DirectionDelta(Direction::Right), Direction::Right, false, 2);
    spawnBullet(center + DirectionDelta(Direction::Down), Direction::Down, false, 2);
    spawnBullet(center + DirectionDelta(Direction::Left), Direction::Left, false, 2);
}

void Game::airStrike()
{
    if (std::rand() % 2 == 0)
    {
        const int column = 2 + std::rand() % (GameMap::Width - 4);
        for (int y = 1; y < GameMap::Height - 1; ++y)
        {
            effects_.push_back(TimedEffect(Vec2(column, y), 18, false, '!', true, EffectType::AirStrikeWarning, DamageType::AirStrike));
        }
    }
    else
    {
        const int row = 2 + std::rand() % (GameMap::Height - 4);
        for (int x = 1; x < GameMap::Width - 1; ++x)
        {
            effects_.push_back(TimedEffect(Vec2(x, row), 18, false, '!', true, EffectType::AirStrikeWarning, DamageType::AirStrike));
        }
    }
}

void Game::spawnEnemyNear(const Vec2& center, EnemyKind kind)
{
    if (enemyCount() + pendingSpawnCount() >= 12)
    {
        return;
    }

    for (int radius = 1; radius <= 4; ++radius)
    {
        const Vec2 candidate = randomAround(center, radius);
        if (canOccupy(candidate, nullptr))
        {
            enemies_.push_back(std::unique_ptr<EnemyTank>(new EnemyTank(candidate, kind, std::rand())));
            return;
        }
    }
}

Vec2 Game::randomAround(const Vec2& center, int radius) const
{
    int x = center.x + (std::rand() % (radius * 2 + 1)) - radius;
    int y = center.y + (std::rand() % (radius * 2 + 1)) - radius;
    if (x < 1) x = 1;
    if (x > GameMap::Width - 2) x = GameMap::Width - 2;
    if (y < 1) y = 1;
    if (y > GameMap::Height - 2) y = GameMap::Height - 2;
    return Vec2(x, y);
}

Vec2 Game::enemyAttractedTarget(const Vec2& enemyPosition) const
{
    if (decoyTicks_ > 0 &&
        IsAligned(enemyPosition, decoyPosition_) &&
        map_.hasLineOfSightOrthogonal(enemyPosition, decoyPosition_))
    {
        return decoyPosition_;
    }

    return player_.position();
}

bool Game::hasLineOfSight(const Vec2& from, const Vec2& to) const
{
    return map_.hasLineOfSightOrthogonal(from, to);
}

std::vector<Vec2> Game::findPath(const Vec2& from, const Vec2& to) const
{
    return map_.findPath(from, to);
}

Vec2 Game::playerPosition() const
{
    return player_.position();
}

int Game::playerLives() const
{
    return player_.lives();
}

int Game::score() const
{
    return score_;
}

int Game::enemyCount() const
{
    int count = 0;
    for (std::size_t i = 0; i < enemies_.size(); ++i)
    {
        if (enemies_[i]->isAlive())
        {
            ++count;
        }
    }
    return count;
}

int Game::wave() const
{
    return wave_;
}

int Game::bossLives() const
{
    for (std::size_t i = 0; i < enemies_.size(); ++i)
    {
        if (enemies_[i]->isBoss())
        {
            return enemies_[i]->lives();
        }
    }

    return 0;
}

int Game::bossMaxLives() const
{
    for (std::size_t i = 0; i < enemies_.size(); ++i)
    {
        if (enemies_[i]->isBoss())
        {
            return enemies_[i]->maxLives();
        }
    }

    return 0;
}

int Game::bombShells() const
{
    return player_.bombShells();
}

int Game::lasers() const
{
    return player_.lasers();
}

int Game::shovels() const
{
    return player_.shovels();
}

int Game::decoys() const
{
    return player_.decoys();
}

int Game::shieldCharges() const
{
    return player_.shields();
}

GameState Game::state() const
{
    return state_;
}

int Game::pendingSpawnCount() const
{
    return static_cast<int>(pendingSpawns_.size());
}

std::string Game::bossStatusText() const
{
    for (std::size_t i = 0; i < enemies_.size(); ++i)
    {
        const EnemyTank& enemy = *enemies_[i];
        if (!enemy.isAlive() || !enemy.isBoss())
        {
            continue;
        }

        std::string text = enemy.isShielded() ? "SHIELD" : "BOSS";
        if (enemy.invisibleActive())
        {
            text += " INVIS";
        }
        if (enemy.reflectActive())
        {
            text += " REFLECT";
        }
        return text;
    }

    return "";
}

bool Game::runSelfTest(std::string& report)
{
    std::vector<std::string> failures;
    std::ostringstream log;

    const auto expect = [&failures](bool condition, const std::string& message)
    {
        if (!condition)
        {
            failures.push_back(message);
        }
    };

    const auto findFirstTile = [this](Tile tileType) -> Vec2
    {
        for (int y = 0; y < GameMap::Height; ++y)
        {
            for (int x = 0; x < GameMap::Width; ++x)
            {
                const Vec2 position(x, y);
                if (map_.tileAt(position) == tileType)
                {
                    return position;
                }
            }
        }
        return Vec2(-1, -1);
    };

    const auto findOpenCross = [this]() -> Vec2
    {
        for (int y = 2; y < GameMap::Height - 2; ++y)
        {
            for (int x = 2; x < GameMap::Width - 2; ++x)
            {
                const Vec2 center(x, y);
                if (!map_.isPassable(center) || map_.isSpawner(center))
                {
                    continue;
                }

                const Vec2 neighbors[4] =
                {
                    center + Vec2(0, -1),
                    center + Vec2(1, 0),
                    center + Vec2(0, 1),
                    center + Vec2(-1, 0)
                };

                bool allOpen = true;
                for (int i = 0; i < 4; ++i)
                {
                    if (!map_.inBounds(neighbors[i]) || !map_.isPassable(neighbors[i]) || map_.isSpawner(neighbors[i]))
                    {
                        allOpen = false;
                        break;
                    }
                }

                if (allOpen)
                {
                    return center;
                }
            }
        }

        return Vec2(-1, -1);
    };

    const auto findSightPair = [this]() -> std::pair<Vec2, Vec2>
    {
        for (int y = 1; y < GameMap::Height - 1; ++y)
        {
            for (int x = 1; x < GameMap::Width - 1; ++x)
            {
                const Vec2 origin(x, y);
                if (!map_.isPassable(origin) || map_.isSpawner(origin))
                {
                    continue;
                }

                const Vec2 candidates[4] =
                {
                    origin + Vec2(0, -3),
                    origin + Vec2(3, 0),
                    origin + Vec2(0, 3),
                    origin + Vec2(-3, 0)
                };

                for (int i = 0; i < 4; ++i)
                {
                    if (!map_.inBounds(candidates[i]) || !map_.isPassable(candidates[i]) || map_.isSpawner(candidates[i]))
                    {
                        continue;
                    }

                    if (map_.hasLineOfSightOrthogonal(origin, candidates[i]))
                    {
                        return std::make_pair(origin, candidates[i]);
                    }
                }
            }
        }

        return std::make_pair(Vec2(-1, -1), Vec2(-1, -1));
    };

    log << "Tank Battle self-test\n";

    std::set<char> terrainGlyphs;
    for (int level = 1; level <= 4; ++level)
    {
        map_.loadLevel(level);
        expect(map_.level() == level, "GameMap level load failed for level " + std::to_string(level));
        expect(!map_.normalSpawners().empty(), "Missing normal spawn point on level " + std::to_string(level));
        if (level >= 2)
        {
            expect(!map_.eliteSpawners().empty(), "Missing elite spawn point on level " + std::to_string(level));
        }
        for (int y = 0; y < GameMap::Height; ++y)
        {
            for (int x = 0; x < GameMap::Width; ++x)
            {
                terrainGlyphs.insert(map_.glyphAt(Vec2(x, y)));
            }
        }
    }
    expect(terrainGlyphs.count('T') > 0, "Trench terrain not found");
    expect(terrainGlyphs.count('%') > 0, "Wooden box terrain not found");
    expect(terrainGlyphs.count('#') > 0, "Wall terrain not found");
    expect(terrainGlyphs.count('~') > 0, "Swamp terrain not found");
    expect(terrainGlyphs.count('N') > 0 && terrainGlyphs.count('E') > 0, "Spawn point terrain not found");
    log << "- Levels and terrain scan complete\n";

    map_.loadLevel(1);
    const Vec2 trench = findFirstTile(Tile::Trench);
    const Vec2 box = findFirstTile(Tile::WoodenBox);
    const Vec2 wall = findFirstTile(Tile::Wall);
    const Vec2 normalSpawner = findFirstTile(Tile::NormalSpawnPoint);
    const Vec2 baseTile = findFirstTile(Tile::Base);
    const Vec2 swamp = findFirstTile(Tile::Swamp);
    expect(trench.x >= 0, "Unable to locate trench tile for test");
    expect(box.x >= 0, "Unable to locate wooden box tile for test");
    expect(wall.x >= 0, "Unable to locate wall tile for test");
    expect(normalSpawner.x >= 0, "Unable to locate spawn tile for test");
    expect(baseTile.x >= 0, "Unable to locate base tile for test");
    expect(swamp.x >= 0, "Unable to locate swamp tile for test");

    if (trench.x >= 0)
    {
        player_ = PlayerTank(trench);
        playerInTrench_ = true;
        const int hpBefore = player_.lives();
        damageTankAt(trench, false, DamageType::NormalShell, Direction::Up);
        expect(player_.lives() == hpBefore, "Trench did not block enemy normal shell");
        damageTankAt(trench, false, DamageType::BombExplosion, Direction::Up);
        expect(player_.lives() == hpBefore - 1, "Bomb damage should still hit player in trench");
    }

    expect(swamp.x >= 0 && map_.slowsMovement(swamp), "Swamp tile did not report slow movement");

    if (box.x >= 0)
    {
        map_.loadLevel(1);
        expect(!map_.damageTile(box, false, DamageType::NormalShell), "Enemy normal shell should not break wooden box");
        expect(map_.tileAt(box) == Tile::WoodenBox, "Wooden box changed after enemy normal shell");
        expect(map_.damageTile(box, true, DamageType::NormalShell), "Player normal shell should break wooden box");
        expect(map_.tileAt(box) == Tile::Empty, "Wooden box not cleared after player damage");
    }

    map_.loadLevel(1);
    const Vec2 emptyCell = map_.randomOpenCell();
    expect(buildTrench(emptyCell), "Shovel trench build failed on open cell");
    if (normalSpawner.x >= 0)
    {
        expect(!buildTrench(normalSpawner), "Spawner tile should not become trench");
    }
    if (wall.x >= 0)
    {
        expect(!buildTrench(wall), "Wall tile should not become trench");
    }
    log << "- Terrain interaction checks complete\n";

    for (int i = 0; i < 5; ++i)
    {
        const PowerUpType type = static_cast<PowerUpType>(i);
        PowerUp powerUp(Vec2(1 + i, 1), type);
        expect(powerUp.glyph() != '?', "Power-up glyph mapping failed for index " + std::to_string(i));
    }
    reset();
    powerUps_.clear();
    powerUps_.push_back(std::unique_ptr<PowerUp>(new PowerUp(player_.position(), PowerUpType::Shield)));
    powerUps_.push_back(std::unique_ptr<PowerUp>(new PowerUp(player_.position(), PowerUpType::BombShell)));
    powerUps_.push_back(std::unique_ptr<PowerUp>(new PowerUp(player_.position(), PowerUpType::Laser)));
    powerUps_.push_back(std::unique_ptr<PowerUp>(new PowerUp(player_.position(), PowerUpType::Shovel)));
    powerUps_.push_back(std::unique_ptr<PowerUp>(new PowerUp(player_.position(), PowerUpType::Decoy)));
    collectPowerUps();
    expect(player_.isShielded(), "Shield power-up was not collected");
    expect(player_.bombShells() == 1, "Bomb shell power-up was not collected");
    expect(player_.lasers() == 1, "Laser power-up was not collected");
    expect(player_.shovels() == 1, "Shovel power-up was not collected");
    expect(player_.decoys() == 1, "Decoy power-up was not collected");

    const int shieldedLives = player_.lives();
    damageTankAt(player_.position(), false, DamageType::NormalShell, Direction::Up);
    expect(player_.lives() == shieldedLives, "Shield did not block first normal hit");
    damageTankAt(player_.position(), false, DamageType::NormalShell, Direction::Up);
    expect(player_.lives() == shieldedLives - 1, "Shield was not consumed after blocking");

    map_.loadLevel(1);
    if (box.x >= 0)
    {
        explodeArea(box, true);
        expect(map_.tileAt(box) == Tile::Empty, "Bomb-style area attack did not destroy wooden box");
    }

    map_.loadLevel(1);
    const Vec2 trenchBuildCell = map_.randomOpenCell();
    expect(buildTrench(trenchBuildCell), "Shovel-style trench build failed after pickup");

    effects_.clear();
    fireLaser(Vec2(5, 5), Direction::Right, true);
    bool sawPlayerLaserTrace = false;
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        if (effects_[i].type == EffectType::LaserTrace && effects_[i].fromPlayer)
        {
            sawPlayerLaserTrace = true;
            break;
        }
    }
    expect(sawPlayerLaserTrace, "Laser power-up usage did not create player laser trace");

    Vec2 decoyCell(-1, -1);
    Vec2 alignedEnemy(-1, -1);
    for (int y = 1; y < GameMap::Height - 1 && decoyCell.x < 0; ++y)
    {
        for (int x = 1; x < GameMap::Width - 1 && decoyCell.x < 0; ++x)
        {
            const Vec2 candidate(x, y);
            if (!map_.isPassable(candidate) || map_.isSpawner(candidate))
            {
                continue;
            }

            const Vec2 targetA(x, y - 2);
            const Vec2 targetB(x, y + 2);
            if (map_.inBounds(targetA) && map_.isPassable(targetA) && map_.hasLineOfSightOrthogonal(candidate, targetA))
            {
                decoyCell = candidate;
                alignedEnemy = targetA;
            }
            else if (map_.inBounds(targetB) && map_.isPassable(targetB) && map_.hasLineOfSightOrthogonal(candidate, targetB))
            {
                decoyCell = candidate;
                alignedEnemy = targetB;
            }
        }
    }
    expect(decoyCell.x >= 0, "Unable to find valid decoy test cell");
    if (decoyCell.x >= 0)
    {
        placeDecoy(decoyCell);
        expect(enemyAttractedTarget(alignedEnemy) == decoyCell, "Decoy usage did not retarget aligned enemy");
    }
    log << "- Power-up pickup and usage checks complete\n";

    const EnemyKind roster[8] =
    {
        EnemyKind::Scout,
        EnemyKind::Armor,
        EnemyKind::LaserGuard,
        EnemyKind::Bomber,
        EnemyKind::Kamikaze,
        EnemyKind::StageTwoBoss,
        EnemyKind::StageThreeBoss,
        EnemyKind::FinalBoss
    };
    std::set<int> livesSet;
    std::set<int> bulletSet;
    std::set<int> moveSet;
    for (int i = 0; i < 8; ++i)
    {
        EnemyTank enemy(Vec2(3 + i, 3), roster[i], 100 + i);
        livesSet.insert(enemy.maxLives());
        bulletSet.insert(enemy.bulletSpeed());
        moveSet.insert(static_cast<int>(enemy.moveSpeedValue() * 1000.0));
    }
    expect(livesSet.size() == 8, "Enemy max lives are not unique across 8 enemy/boss kinds");
    expect(bulletSet.size() == 8, "Enemy bullet speeds are not unique across 8 enemy/boss kinds");
    expect(moveSet.size() == 8, "Enemy move speeds are not unique across 8 enemy/boss kinds");
    log << "- Enemy roster stat uniqueness checks complete\n";

    const std::pair<Vec2, Vec2> guardSightPair = findSightPair();
    expect(guardSightPair.first.x >= 0, "Unable to find open sight pair for enemy skill tests");
    enemies_.clear();
    effects_.clear();
    if (guardSightPair.first.x >= 0)
    {
        player_ = PlayerTank(guardSightPair.second);
        enemies_.push_back(std::unique_ptr<EnemyTank>(new EnemyTank(guardSightPair.first, EnemyKind::LaserGuard, 2468)));
    }
    for (int i = 0; i < 60; ++i)
    {
        if (!enemies_.empty())
        {
            enemies_[0]->update(*this);
        }
    }
    bool sawGuardLaser = false;
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        if (effects_[i].type == EffectType::LaserTrace && !effects_[i].fromPlayer)
        {
            sawGuardLaser = true;
            break;
        }
    }
    expect(!enemies_.empty() && enemies_[0]->isShielded(), "LaserGuard did not refresh shield");
    expect(sawGuardLaser, "LaserGuard did not fire line laser");

    reset();
    placeDecoy(player_.position());
    const Vec2 decoyTarget = enemyAttractedTarget(Vec2(player_.position().x, 1));
    expect(decoyTarget == player_.position(), "Decoy did not attract aligned enemy to player cell");

    effects_.clear();
    fireLaser(Vec2(5, 5), Direction::Right, true);
    bool sawLaserTrace = false;
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        if (effects_[i].type == EffectType::LaserTrace)
        {
            sawLaserTrace = true;
            break;
        }
    }
    expect(sawLaserTrace, "Laser trace effect was not created");
    log << "- Decoy and laser effect checks complete\n";

    reset();
    enemies_.clear();
    effects_.clear();
    enemies_.push_back(std::unique_ptr<EnemyTank>(new EnemyTank(player_.position() + Vec2(1, 0), EnemyKind::Kamikaze, 4321)));
    for (int i = 0; i < 60; ++i)
    {
        enemies_[0]->update(*this);
    }
    expect(enemies_[0]->isAlive(), "Kamikaze exploded too early");
    int chargeWarnings = 0;
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        if (effects_[i].type == EffectType::Warning)
        {
            ++chargeWarnings;
        }
    }
    expect(chargeWarnings > 0, "Kamikaze did not create charge warning");
    for (int i = 0; i < 80 && enemies_[0]->isAlive(); ++i)
    {
        enemies_[0]->update(*this);
    }
    expect(!enemies_[0]->isAlive(), "Kamikaze did not explode after full charge");

    reset();
    wave_ = 1;
    enemies_.clear();
    pendingSpawns_.clear();
    effects_.clear();
    spawnRandomEnemy(true);
    expect(!pendingSpawns_.empty(), "Wave 1 spawn queue is empty");
    expect(pendingSpawns_[0].kind == EnemyKind::Scout || pendingSpawns_[0].kind == EnemyKind::Armor,
        "Wave 1 should not queue elite enemies");

    reset();
    wave_ = 3;
    spawnTick_ = 0;
    enemies_.clear();
    pendingSpawns_.clear();
    effects_.clear();
    spawnRandomEnemy(true);
    expect(!pendingSpawns_.empty(), "Elite spawn did not enter pending queue");
    bool sawSpawnWarning = false;
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        if (effects_[i].type == EffectType::SpawnWarning)
        {
            sawSpawnWarning = true;
            break;
        }
    }
    expect(sawSpawnWarning, "Spawn warning effect was not created");
    log << "- Spawn warning checks complete\n";

    reset();
    const Vec2 bossArena = findOpenCross();
    expect(bossArena.x >= 0, "Unable to find open cross cell for boss barrage test");
    enemies_.clear();
    effects_.clear();
    bullets_.clear();
    if (bossArena.x >= 0)
    {
        player_ = PlayerTank(bossArena + Vec2(0, 3));
        enemies_.push_back(std::unique_ptr<EnemyTank>(new EnemyTank(bossArena, EnemyKind::StageTwoBoss, 1357)));
    }
    for (int i = 0; i < 60; ++i)
    {
        if (!enemies_.empty())
        {
            enemies_[0]->update(*this);
        }
    }
    bool sawBossBomb = false;
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        if (effects_[i].type == EffectType::Warning)
        {
            sawBossBomb = true;
            break;
        }
    }
    expect(sawBossBomb, "Stage two boss did not create bomb warning");
    expect(bullets_.size() >= 4, "Stage two boss did not fire machine-gun burst");

    reset();
    state_ = GameState::StageCleared;
    advanceStage();
    expect(wave_ == 2 && state_ == GameState::Running, "AdvanceStage did not move from level 1 to level 2");
    state_ = GameState::StageCleared;
    wave_ = 4;
    advanceStage();
    expect(state_ == GameState::Victory, "AdvanceStage did not end game after level 4");
    reset();
    expect(state_ == GameState::Running, "Reset did not restore running state");
    expect(wave_ == 1, "Reset did not restore wave to 1");
    expect(pendingSpawns_.empty() && effects_.empty() && bullets_.empty(), "Reset did not clear transient objects");
    log << "- Stage transition checks complete\n";

    reset();
    if (baseTile.x >= 0)
    {
        map_.damageTile(baseTile, false, DamageType::BombExplosion);
        checkGameState();
        expect(state_ == GameState::Defeat, "Base destruction did not trigger defeat");
    }

    reset();
    const int startingLives = player_.lives();
    for (int i = 0; i < startingLives; ++i)
    {
        damageTankAt(player_.position(), false, DamageType::BombExplosion, Direction::Up);
    }
    checkGameState();
    expect(state_ == GameState::Defeat, "Player death did not trigger defeat");

    reset();
    enemies_.clear();
    enemies_.push_back(std::unique_ptr<EnemyTank>(new EnemyTank(Vec2(GameMap::Width / 2, 2), EnemyKind::StageThreeBoss, 1234)));
    for (int i = 0; i < 60; ++i)
    {
        enemies_[0]->update(*this);
    }
    expect(enemies_[0]->isBoss(), "Stage three boss test entity missing");
    expect(enemies_[0]->reflectActive() || enemies_[0]->invisibleActive(), "Stage three boss did not activate special status");

    enemies_.clear();
    enemies_.push_back(std::unique_ptr<EnemyTank>(new EnemyTank(Vec2(GameMap::Width / 2, 2), EnemyKind::FinalBoss, 5678)));
    effects_.clear();
    for (int i = 0; i < 70; ++i)
    {
        enemies_[0]->update(*this);
    }
    bool sawAirStrike = false;
    bool sawBossLaser = false;
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        sawAirStrike = sawAirStrike || effects_[i].type == EffectType::AirStrikeWarning;
        sawBossLaser = sawBossLaser || effects_[i].type == EffectType::LaserTrace;
    }
    expect(sawAirStrike, "Final boss did not create air strike warning in self-test");
    expect(sawBossLaser, "Final boss did not create laser trace in self-test");

    enemies_.clear();
    enemies_.push_back(std::unique_ptr<EnemyTank>(new EnemyTank(Vec2(GameMap::Width / 2, 2), EnemyKind::Bomber, 9012)));
    effects_.clear();
    for (int i = 0; i < 60; ++i)
    {
        enemies_[0]->update(*this);
    }
    int bombWarnings = 0;
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        if (effects_[i].type == EffectType::Warning)
        {
            ++bombWarnings;
        }
    }
    expect(bombWarnings >= 4, "Bomber did not create four-direction warning set");
    log << "- Advanced enemy and boss skill checks complete\n";

    report = log.str();
    if (failures.empty())
    {
        report += "RESULT: PASS\n";
        return true;
    }

    report += "RESULT: FAIL\n";
    for (std::size_t i = 0; i < failures.size(); ++i)
    {
        report += "- " + failures[i] + "\n";
    }
    return false;
}

const std::vector<std::unique_ptr<EnemyTank> >& Game::enemies() const
{
    return enemies_;
}

const std::vector<std::unique_ptr<Bullet> >& Game::bullets() const
{
    return bullets_;
}

const std::vector<std::unique_ptr<PowerUp> >& Game::powerUps() const
{
    return powerUps_;
}

const std::vector<TimedEffect>& Game::effects() const
{
    return effects_;
}

const PlayerTank& Game::player() const
{
    return player_;
}

char Game::mapGlyphAt(const Vec2& position) const
{
    return map_.glyphAt(position);
}

void Game::draw(std::vector<std::string>& buffer) const
{
    map_.draw(buffer);

    for (std::size_t i = 0; i < powerUps_.size(); ++i)
    {
        const PowerUp& powerUp = *powerUps_[i];
        if (powerUp.isAlive() && map_.inBounds(powerUp.position()))
        {
            buffer[powerUp.position().y][powerUp.position().x] = powerUp.glyph();
        }
    }

    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        if (map_.inBounds(effects_[i].position))
        {
            buffer[effects_[i].position.y][effects_[i].position.x] = effects_[i].glyph;
        }
    }

    if (player_.isAlive() && map_.inBounds(player_.position()))
    {
        buffer[player_.position().y][player_.position().x] = player_.isShielded() ? '@' : player_.glyph();
    }

    for (std::size_t i = 0; i < enemies_.size(); ++i)
    {
        const EnemyTank& enemy = *enemies_[i];
        if (enemy.isAlive() && enemy.isVisible() && map_.inBounds(enemy.position()))
        {
            buffer[enemy.position().y][enemy.position().x] = enemy.glyph();
        }
    }

    for (std::size_t i = 0; i < bullets_.size(); ++i)
    {
        const Bullet& bullet = *bullets_[i];
        if (bullet.isAlive() && map_.inBounds(bullet.position()))
        {
            buffer[bullet.position().y][bullet.position().x] = bullet.glyph();
        }
    }
}

void Game::update()
{
    if (map_.tileAt(player_.position()) != Tile::Trench)
    {
        playerInTrench_ = false;
    }

    if (map_.slowsMovement(player_.position()))
    {
        playerMoveSkip_ = 1 - playerMoveSkip_;
        if (playerMoveSkip_ == 0)
        {
            player_.update(*this);
        }
        else
        {
            player_.reduceFireCooldown();
        }
    }
    else
    {
        player_.update(*this);
    }

    collectPowerUps();

    for (std::size_t i = 0; i < enemies_.size(); ++i)
    {
        if (enemies_[i]->isAlive())
        {
            enemies_[i]->update(*this);
        }
    }

    const std::size_t bulletCount = bullets_.size();
    for (std::size_t i = 0; i < bulletCount; ++i)
    {
        if (bullets_[i]->isAlive())
        {
            bullets_[i]->update(*this);
        }
    }

    updateEffects();
    updatePendingSpawns();

    int spawnLimit = 90;
    if (difficulty_ == Difficulty::Easy) spawnLimit = 125;
    if (difficulty_ == Difficulty::Hard) spawnLimit = 65;
    if (++spawnTick_ > spawnLimit)
    {
        spawnTick_ = 0;
        spawnRandomEnemy(std::rand() % 3 == 0);
    }

    int powerLimit = 130;
    if (difficulty_ == Difficulty::Easy) powerLimit = 95;
    if (difficulty_ == Difficulty::Hard) powerLimit = 165;
    if (++powerUpTick_ > powerLimit)
    {
        powerUpTick_ = 0;
        spawnPowerUp();
    }

    if (decoyTicks_ > 0)
    {
        --decoyTicks_;
    }
    else
    {
        decoyPosition_ = Vec2();
    }

    removeDeadEntities();

    if (enemyCount() == 0 && pendingSpawns_.empty() && state_ == GameState::Running)
    {
        if (wave_ >= 4)
        {
            state_ = GameState::Victory;
        }
        else
        {
            state_ = GameState::StageCleared;
        }
    }

    checkGameState();
}

void Game::handleGlobalInput()
{
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
    {
        state_ = GameState::Quit;
        return;
    }

    if (GetAsyncKeyState('R') & 0x8000)
    {
        reset();
    }
}

void Game::spawnWave()
{
    map_.loadLevel(wave_);
    enemies_.clear();
    bullets_.clear();
    effects_.clear();
    pendingSpawns_.clear();
    powerUps_.clear();

    spawnEnemyNear(Vec2(3, 1), EnemyKind::Scout);
    spawnEnemyNear(Vec2(GameMap::Width - 4, 1), EnemyKind::Armor);
    if (difficulty_ == Difficulty::Hard)
    {
        spawnEnemyNear(Vec2(GameMap::Width / 2, 1), EnemyKind::Armor);
    }

    if (wave_ == 2)
    {
        spawnEnemyNear(Vec2(GameMap::Width / 2, 1), EnemyKind::LaserGuard);
        spawnEnemyNear(Vec2(GameMap::Width / 2, 3), EnemyKind::StageTwoBoss);
    }
    else if (wave_ == 3)
    {
        spawnEnemyNear(Vec2(7, 3), EnemyKind::LaserGuard);
        spawnEnemyNear(Vec2(GameMap::Width - 8, 3), EnemyKind::Bomber);
        spawnEnemyNear(Vec2(GameMap::Width / 2, 2), EnemyKind::StageThreeBoss);
    }
    else if (wave_ == 4)
    {
        spawnEnemyNear(Vec2(7, 3), EnemyKind::LaserGuard);
        spawnEnemyNear(Vec2(GameMap::Width - 8, 3), EnemyKind::Bomber);
        spawnEnemyNear(Vec2(GameMap::Width / 2, 4), EnemyKind::Kamikaze);
        spawnEnemyNear(Vec2(GameMap::Width / 2, 2), EnemyKind::FinalBoss);
    }
}

void Game::spawnRandomEnemy(bool elite)
{
    if (wave_ <= 1)
    {
        elite = false;
    }

    if (enemyCount() + pendingSpawnCount() > 8)
    {
        return;
    }

    std::vector<Vec2> spawners = elite ? map_.eliteSpawners() : map_.normalSpawners();
    if (spawners.empty())
    {
        return;
    }

    const Vec2 spawn = spawners[std::rand() % spawners.size()];
    EnemyKind kind = EnemyKind::Scout;
    if (elite)
    {
        const EnemyKind eliteKinds[3] =
        {
            EnemyKind::LaserGuard,
            EnemyKind::Bomber,
            EnemyKind::Kamikaze
        };
        int maxElite = wave_ - 1;
        if (maxElite < 1) maxElite = 1;
        if (maxElite > 3) maxElite = 3;
        kind = eliteKinds[std::rand() % maxElite];
    }
    else
    {
        kind = (std::rand() % 2 == 0) ? EnemyKind::Scout : EnemyKind::Armor;
    }

    effects_.push_back(TimedEffect(spawn, 18, false, '?', false, EffectType::SpawnWarning, DamageType::NormalShell));
    pendingSpawns_.push_back(PendingSpawn(spawn, kind, 18));
}

void Game::spawnPowerUp()
{
    if (state_ != GameState::Running)
    {
        return;
    }

    if (powerUps_.size() >= 5)
    {
        return;
    }

    const Vec2 position = map_.randomOpenCell();
    if (isOccupiedByTank(position, nullptr))
    {
        return;
    }

    const PowerUpType type = static_cast<PowerUpType>(std::rand() % 5);
    powerUps_.push_back(std::unique_ptr<PowerUp>(new PowerUp(position, type)));
}

void Game::updatePendingSpawns()
{
    for (std::size_t i = 0; i < pendingSpawns_.size(); ++i)
    {
        --pendingSpawns_[i].ticks;
        if (pendingSpawns_[i].ticks <= 0)
        {
            if (enemyCount() >= 10)
            {
                pendingSpawns_[i].ticks = 10;
                continue;
            }

            spawnEnemyNear(pendingSpawns_[i].center, pendingSpawns_[i].kind);
            pendingSpawns_[i].ticks = -999;
        }
    }

    pendingSpawns_.erase(
        std::remove_if(pendingSpawns_.begin(), pendingSpawns_.end(),
            [](const PendingSpawn& spawn) { return spawn.ticks < 0; }),
        pendingSpawns_.end());
}

void Game::updateEffects()
{
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        --effects_[i].ticks;
        if (effects_[i].ticks == 0 && effects_[i].explodes)
        {
            if (effects_[i].damageType == DamageType::AirStrike)
            {
                damageTankAt(effects_[i].position, effects_[i].fromPlayer, DamageType::AirStrike, Direction::Up);
            }
            else if (effects_[i].damageType == DamageType::SuicideExplosion)
            {
                for (int y = effects_[i].position.y - 1; y <= effects_[i].position.y + 1; ++y)
                {
                    for (int x = effects_[i].position.x - 1; x <= effects_[i].position.x + 1; ++x)
                    {
                        const Vec2 position(x, y);
                        if (map_.inBounds(position))
                        {
                            map_.damageTile(position, effects_[i].fromPlayer, DamageType::SuicideExplosion);
                            damageTankAt(position, effects_[i].fromPlayer, DamageType::SuicideExplosion, Direction::Up);
                        }
                    }
                }
            }
            else
            {
                explodeArea(effects_[i].position, effects_[i].fromPlayer);
            }
        }
    }
}

void Game::collectPowerUps()
{
    if (state_ != GameState::Running)
    {
        return;
    }

    for (std::size_t i = 0; i < powerUps_.size(); ++i)
    {
        if (powerUps_[i]->isAlive() && powerUps_[i]->position() == player_.position())
        {
            switch (powerUps_[i]->type())
            {
            case PowerUpType::Shield:
                player_.addShield(160);
                break;
            case PowerUpType::BombShell:
                player_.addBombShell();
                break;
            case PowerUpType::Laser:
                player_.addLaser();
                break;
            case PowerUpType::Shovel:
                player_.addShovel();
                break;
            case PowerUpType::Decoy:
                player_.addDecoy();
                break;
            }

            powerUps_[i]->destroy();
        }
    }
}

void Game::removeDeadEntities()
{
    enemies_.erase(
        std::remove_if(enemies_.begin(), enemies_.end(),
            [](const std::unique_ptr<EnemyTank>& enemy) { return !enemy->isAlive(); }),
        enemies_.end());

    bullets_.erase(
        std::remove_if(bullets_.begin(), bullets_.end(),
            [](const std::unique_ptr<Bullet>& bullet) { return !bullet->isAlive(); }),
        bullets_.end());

    powerUps_.erase(
        std::remove_if(powerUps_.begin(), powerUps_.end(),
            [](const std::unique_ptr<PowerUp>& powerUp) { return !powerUp->isAlive(); }),
        powerUps_.end());

    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
            [](const TimedEffect& effect) { return effect.ticks <= 0; }),
        effects_.end());
}

bool Game::isOccupiedByTank(const Vec2& position, const Entity* requester) const
{
    if (&player_ != requester && player_.isAlive() && player_.position() == position)
    {
        return true;
    }

    for (std::size_t i = 0; i < enemies_.size(); ++i)
    {
        if (enemies_[i].get() != requester && enemies_[i]->isAlive() && enemies_[i]->position() == position)
        {
            return true;
        }
    }

    return false;
}

void Game::damageTankAt(const Vec2& position, bool fromPlayer, DamageType damageType, Direction attackDirection)
{
    if (fromPlayer)
    {
        for (std::size_t i = 0; i < enemies_.size(); ++i)
        {
            if (enemies_[i]->isAlive() && enemies_[i]->position() == position)
            {
                if (enemies_[i]->shouldReflectAttack(attackDirection, damageType))
                {
                    spawnBullet(position + DirectionDelta(OppositeDirection(attackDirection)),
                        OppositeDirection(attackDirection), false, 2);
                    return;
                }

                if (enemies_[i]->takeHit())
                {
                    score_ += enemies_[i]->scoreValue();
                }
                return;
            }
        }
    }
    else if (player_.isAlive() && player_.position() == position)
    {
        if (damageType == DamageType::NormalShell && playerInTrench_)
        {
            return;
        }
        player_.takeHit();
    }
}

void Game::checkGameState()
{
    if (!player_.isAlive() || map_.isBaseDestroyed())
    {
        state_ = GameState::Defeat;
    }
}
