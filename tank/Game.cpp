#include "Game.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <thread>
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
      frameAccumulator_(0)
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
    map_.reset();
    player_ = PlayerTank(Vec2(GameMap::Width / 2, GameMap::Height - 4));
    enemies_.clear();
    bullets_.clear();
    powerUps_.clear();
    effects_.clear();
    state_ = GameState::Running;
    score_ = 0;
    wave_ = 1;
    spawnTick_ = 0;
    powerUpTick_ = 0;
    playerMoveSkip_ = 0;
    decoyTicks_ = 0;
    frameAccumulator_ = 0;
    spawnWave();
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

bool Game::resolveTankCollision(const Entity* mover, const Vec2& target)
{
    if (mover == &player_)
    {
        for (std::size_t i = 0; i < enemies_.size(); ++i)
        {
            if (enemies_[i]->isAlive() && enemies_[i]->position() == target)
            {
                const int collisionDamage = enemies_[i]->lives();
                player_.takeDamage(collisionDamage);
                enemies_[i]->destroy();
                score_ += enemies_[i]->scoreValue();
                return true;
            }
        }
    }
    else if (player_.isAlive() && player_.position() == target && map_.tileAt(player_.position()) != Tile::Trench)
    {
        for (std::size_t i = 0; i < enemies_.size(); ++i)
        {
            if (enemies_[i].get() == mover && enemies_[i]->isAlive())
            {
                player_.takeDamage(enemies_[i]->lives());
                enemies_[i]->destroy();
                return true;
            }
        }
    }

    return false;
}

bool Game::spawnBullet(const Vec2& position, Direction direction, bool fromPlayer, int speed)
{
    if (!map_.inBounds(position))
    {
        return false;
    }

    if (map_.isBulletBlocked(position))
    {
        map_.damageTile(position);
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
            map_.damageTile(cell);
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
            map_.damageTile(target);
        }

        return true;
    }

    if (!bullet.fromPlayer() && map_.tileAt(player_.position()) == Tile::Trench)
    {
        return false;
    }

    if (damageTankAt(target, bullet.fromPlayer()))
    {
        return true;
    }

    return !map_.isPassable(target);
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
    return map_.tileAt(player_.position()) == Tile::Trench;
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
                map_.damageTile(position);
                damageTankAt(position, fromPlayer);
            }
        }
    }
}

void Game::fireLaser(const Vec2& start, Direction direction, bool fromPlayer)
{
    Vec2 position = start + DirectionDelta(direction);
    const char laserGlyph = (direction == Direction::Left || direction == Direction::Right) ? '-' : '|';
    while (map_.inBounds(position))
    {
        effects_.push_back(TimedEffect(position, 8, fromPlayer, laserGlyph, false));
        damageTankAt(position, fromPlayer);
        position = position + DirectionDelta(direction);
    }
}

void Game::markDanger(const Vec2& center, int ticks, bool fromPlayer)
{
    effects_.push_back(TimedEffect(center, ticks, fromPlayer, '!', true));
}

void Game::placeDecoy(const Vec2& position)
{
    if (!map_.isPassable(position))
    {
        return;
    }

    decoyPosition_ = position;
    decoyTicks_ = 15 * 14;
    effects_.push_back(TimedEffect(position, decoyTicks_, true, 'H', false));
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
            effects_.push_back(TimedEffect(Vec2(column, y), 18, false, '!', true));
        }
    }
    else
    {
        const int row = 2 + std::rand() % (GameMap::Height - 4);
        for (int x = 1; x < GameMap::Width - 1; ++x)
        {
            effects_.push_back(TimedEffect(Vec2(x, row), 18, false, '!', true));
        }
    }
}

void Game::spawnEnemyNear(const Vec2& center, EnemyKind kind)
{
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
    if (decoyTicks_ > 0 && (enemyPosition.x == decoyPosition_.x || enemyPosition.y == decoyPosition_.y))
    {
        return decoyPosition_;
    }

    return player_.position();
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
        if (enemy.isAlive() && map_.inBounds(enemy.position()))
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

    removeDeadEntities();

    if (enemyCount() == 0 && state_ == GameState::Running)
    {
        ++spawnTick_;
        if (spawnTick_ > 35)
        {
            ++wave_;
            spawnTick_ = 0;
            if (wave_ > 4)
            {
                state_ = GameState::Victory;
            }
            else
            {
                spawnWave();
            }
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
    enemies_.clear();
    bullets_.clear();
    effects_.clear();

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
    if (enemyCount() > 8)
    {
        return;
    }

    std::vector<Vec2> spawners = elite ? map_.eliteSpawners() : map_.normalSpawners();
    if (spawners.empty())
    {
        return;
    }

    const Vec2 spawn = spawners[std::rand() % spawners.size()];
    effects_.push_back(TimedEffect(spawn, 12, false, '?', false));
    EnemyKind kind = EnemyKind::Scout;
    if (elite)
    {
        int maxElite = wave_;
        if (maxElite < 1) maxElite = 1;
        if (maxElite > 3) maxElite = 3;
        kind = static_cast<EnemyKind>(2 + std::rand() % maxElite);
    }
    else
    {
        kind = (std::rand() % 2 == 0) ? EnemyKind::Scout : EnemyKind::Armor;
    }

    spawnEnemyNear(spawn, kind);
}

void Game::spawnPowerUp()
{
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

void Game::updateEffects()
{
    for (std::size_t i = 0; i < effects_.size(); ++i)
    {
        --effects_[i].ticks;
        if (effects_[i].ticks == 0 && effects_[i].explodes)
        {
            explodeArea(effects_[i].position, effects_[i].fromPlayer);
        }
    }
}

void Game::collectPowerUps()
{
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

bool Game::damageTankAt(const Vec2& position, bool fromPlayer)
{
    if (fromPlayer)
    {
        for (std::size_t i = 0; i < enemies_.size(); ++i)
        {
            if (enemies_[i]->isAlive() && enemies_[i]->position() == position)
            {
                if (enemies_[i]->takeHit())
                {
                    score_ += enemies_[i]->scoreValue();
                }
                return true;
            }
        }
    }
    else if (player_.isAlive() && player_.position() == position && map_.tileAt(player_.position()) != Tile::Trench)
    {
        player_.takeHit();
        return true;
    }

    return false;
}

void Game::checkGameState()
{
    if (!player_.isAlive() || map_.isBaseDestroyed())
    {
        state_ = GameState::Defeat;
    }
}
