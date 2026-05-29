#include "Tank.h"

#include <cstdlib>
#include <windows.h>

#include "Game.h"

Tank::Tank(const Vec2& position, Direction direction, int fireCooldownTicks)
    : Entity(position),
      direction_(direction),
      fireCooldown_(0),
      fireCooldownTicks_(fireCooldownTicks),
      lives_(1),
      maxLives_(1),
      bulletSpeed_(1),
      moveInterval_(7),
      shieldTicks_(0),
      kind_(EnemyKind::Scout),
      moveSpeed_(0.16)
{
}

Direction Tank::direction() const
{
    return direction_;
}

EnemyKind Tank::enemyKind() const
{
    return kind_;
}

int Tank::lives() const
{
    return lives_;
}

int Tank::maxLives() const
{
    return maxLives_;
}

bool Tank::isShielded() const
{
    return shieldTicks_ > 0;
}

void Tank::setStats(int lives, int fireCooldownTicks, int bulletSpeed, int moveInterval, EnemyKind kind)
{
    lives_ = lives;
    maxLives_ = lives;
    fireCooldownTicks_ = fireCooldownTicks;
    bulletSpeed_ = bulletSpeed;
    moveInterval_ = moveInterval;
    kind_ = kind;
}

void Tank::setShield(int ticks)
{
    if (ticks > shieldTicks_)
    {
        shieldTicks_ = ticks;
    }
}

void Tank::resetFireCooldown()
{
    fireCooldown_ = fireCooldownTicks_;
}

void Tank::reduceFireCooldown()
{
    if (fireCooldown_ > 0)
    {
        --fireCooldown_;
    }

    if (shieldTicks_ > 0)
    {
        --shieldTicks_;
    }
}

bool Tank::canFire() const
{
    return fireCooldown_ <= 0;
}

bool Tank::takeHit()
{
    return takeDamage(1);
}

bool Tank::takeDamage(int amount)
{
    if (shieldTicks_ > 0)
    {
        shieldTicks_ = 0;
        return false;
    }

    lives_ -= amount;
    if (lives_ <= 0)
    {
        destroy();
        return true;
    }

    return false;
}

char Tank::glyph() const
{
    return DirectionGlyph(direction_);
}

bool Tank::tryMove(Game& game, Direction direction)
{
    direction_ = direction;
    const FloatVec2 delta = DirectionFloatDelta(direction);
    const FloatVec2 target(precisePosition_.x + delta.x * moveSpeed_, precisePosition_.y + delta.y * moveSpeed_);
    if (game.canOccupyPrecise(target, this))
    {
        setPrecisePosition(target);
        return true;
    }

    game.resolveTankCollision(this, ToCell(target));
    return false;
}

void Tank::tryFire(Game& game, bool fromPlayer)
{
    if (!canFire())
    {
        return;
    }

    const FloatVec2 delta = DirectionFloatDelta(direction_);
    const FloatVec2 muzzle(precisePosition_.x + delta.x * 0.58, precisePosition_.y + delta.y * 0.58);
    if (game.spawnBullet(muzzle, direction_, fromPlayer, bulletSpeed_))
    {
        resetFireCooldown();
    }
}

PlayerTank::PlayerTank(const Vec2& position)
    : Tank(position, Direction::Up, 10),
      bombShells_(0),
      lasers_(0),
      shovels_(0),
      decoys_(0)
{
    setStats(5, 10, 2, 4, EnemyKind::Scout);
    moveSpeed_ = 0.095;
}

void PlayerTank::addShield(int ticks)
{
    setShield(ticks);
}

void PlayerTank::addBombShell()
{
    ++bombShells_;
}

void PlayerTank::addLaser()
{
    ++lasers_;
}

void PlayerTank::addShovel()
{
    ++shovels_;
}

void PlayerTank::addDecoy()
{
    ++decoys_;
}

int PlayerTank::shields() const
{
    return isShielded() ? 1 : 0;
}

int PlayerTank::bombShells() const
{
    return bombShells_;
}

int PlayerTank::lasers() const
{
    return lasers_;
}

int PlayerTank::shovels() const
{
    return shovels_;
}

int PlayerTank::decoys() const
{
    return decoys_;
}

void PlayerTank::update(Game& game)
{
    reduceFireCooldown();

    if (GetAsyncKeyState('J') & 0x8000)
    {
        tryFire(game, true);
    }

    if (GetAsyncKeyState('W') & 0x8000)
    {
        tryMove(game, Direction::Up);
    }
    else if (GetAsyncKeyState('D') & 0x8000)
    {
        tryMove(game, Direction::Right);
    }
    else if (GetAsyncKeyState('S') & 0x8000)
    {
        tryMove(game, Direction::Down);
    }
    else if (GetAsyncKeyState('A') & 0x8000)
    {
        tryMove(game, Direction::Left);
    }

    if ((GetAsyncKeyState('F') & 0x8000) && bombShells_ > 0)
    {
        --bombShells_;
        game.explodeArea(position_ + DirectionDelta(direction_), true);
    }

    if ((GetAsyncKeyState('E') & 0x8000) && lasers_ > 0)
    {
        --lasers_;
        game.fireLaser(position_, direction_, true);
    }

    if ((GetAsyncKeyState('Q') & 0x8000) && shovels_ > 0)
    {
        if (game.buildTrench(position_))
        {
            --shovels_;
        }
    }

    if ((GetAsyncKeyState('T') & 0x8000))
    {
        game.tryEnterTrench();
    }

    if ((GetAsyncKeyState('G') & 0x8000) && decoys_ > 0)
    {
        --decoys_;
        game.placeDecoy(position_ + DirectionDelta(direction_));
    }
}

EnemyTank::EnemyTank(const Vec2& position, EnemyKind kind, int seed)
    : Tank(position, Direction::Down, 22),
      aiCounter_(0),
      seed_(seed),
      skillCooldown_(40),
      moveDebt_(0),
      turnCooldown_(0),
      fireTimer_(0)
{
    direction_ = static_cast<Direction>((std::rand() + seed_) % 4);
    chooseTurnCooldown();
    fireTimer_ = fixedFireInterval() / 2 + (std::rand() % 20);

    switch (kind)
    {
    case EnemyKind::Scout:
        setStats(1, 20, 1, 3, kind);
        moveSpeed_ = 0.105;
        break;
    case EnemyKind::Armor:
        setStats(2, 28, 2, 4, kind);
        moveSpeed_ = 0.085;
        break;
    case EnemyKind::LaserGuard:
        setStats(3, 26, 3, 4, kind);
        setShield(80);
        moveSpeed_ = 0.080;
        break;
    case EnemyKind::Bomber:
        setStats(4, 32, 1, 5, kind);
        moveSpeed_ = 0.074;
        break;
    case EnemyKind::Kamikaze:
        setStats(5, 35, 4, 2, kind);
        moveSpeed_ = 0.120;
        break;
    case EnemyKind::StageTwoBoss:
        setStats(8, 16, 5, 4, kind);
        setShield(100);
        moveSpeed_ = 0.068;
        break;
    case EnemyKind::StageThreeBoss:
        setStats(10, 14, 6, 4, kind);
        moveSpeed_ = 0.062;
        break;
    case EnemyKind::FinalBoss:
        setStats(16, 12, 7, 3, kind);
        setShield(120);
        moveSpeed_ = 0.056;
        break;
    }
}

void EnemyTank::update(Game& game)
{
    reduceFireCooldown();
    ++aiCounter_;
    ++fireTimer_;
    if (skillCooldown_ > 0)
    {
        --skillCooldown_;
    }

    if (skillCooldown_ <= 0)
    {
        useSkill(game);
    }

    if (game.isSlowed(position_))
    {
        moveDebt_ = 1 - moveDebt_;
        if (moveDebt_ == 1)
        {
            return;
        }
    }

    if (fireTimer_ >= fixedFireInterval())
    {
        fireTimer_ = 0;
        if (kind_ == EnemyKind::LaserGuard)
        {
            setShield(45);
            game.fireLaser(position_, direction_, false);
        }
        else
        {
            tryFire(game, false);
        }
    }

    if (turnCooldown_ > 0)
    {
        --turnCooldown_;
    }

    if (turnCooldown_ <= 0)
    {
        chooseNewDirection(game, false);
        chooseTurnCooldown();
    }

    if (aiCounter_ % moveInterval_ == 0)
    {
        const Vec2 target = game.enemyAttractedTarget(position_);
        const int dx = target.x - position_.x;
        const int dy = target.y - position_.y;

        if (kind_ == EnemyKind::Kamikaze)
        {
            Direction chase = std::abs(dx) > std::abs(dy)
                ? (dx > 0 ? Direction::Right : Direction::Left)
                : (dy > 0 ? Direction::Down : Direction::Up);
            if (!tryMove(game, chase))
            {
                chooseNewDirection(game, true);
            }
            if (ManhattanDistance(position_, game.playerPosition()) <= 1)
            {
                game.markDanger(position_, 28, false);
                if (aiCounter_ % 28 == 0)
                {
                    game.explodeArea(position_, false);
                    destroy();
                }
            }
        }
        else
        {
            if (!tryMove(game, direction_))
            {
                chooseNewDirection(game, true);
                chooseTurnCooldown();
                tryMove(game, direction_);
            }
        }
    }
}

char EnemyTank::glyph() const
{
    switch (kind_)
    {
    case EnemyKind::Scout:
        return '1';
    case EnemyKind::Armor:
        return '2';
    case EnemyKind::LaserGuard:
        return isShielded() ? 'L' : '3';
    case EnemyKind::Bomber:
        return '4';
    case EnemyKind::Kamikaze:
        return '5';
    case EnemyKind::StageTwoBoss:
        return 'B';
    case EnemyKind::StageThreeBoss:
        return 'C';
    case EnemyKind::FinalBoss:
        return 'Z';
    default:
        return 'E';
    }
}

int EnemyTank::scoreValue() const
{
    return isBoss() ? 800 : 100 + static_cast<int>(kind_) * 40;
}

bool EnemyTank::isBoss() const
{
    return kind_ == EnemyKind::StageTwoBoss || kind_ == EnemyKind::StageThreeBoss || kind_ == EnemyKind::FinalBoss;
}

void EnemyTank::useSkill(Game& game)
{
    switch (kind_)
    {
    case EnemyKind::LaserGuard:
        setShield(70);
        game.fireLaser(position_, direction_, false);
        skillCooldown_ = 95;
        break;
    case EnemyKind::Bomber:
        game.markDanger(game.randomAround(position_, 5), 24, false);
        skillCooldown_ = 110;
        break;
    case EnemyKind::StageTwoBoss:
        setShield(80);
        game.markDanger(game.playerPosition(), 26, false);
        skillCooldown_ = 80;
        break;
    case EnemyKind::StageThreeBoss:
        game.machineGun(position_);
        game.spawnEnemyNear(position_, EnemyKind::LaserGuard);
        skillCooldown_ = 90;
        break;
    case EnemyKind::FinalBoss:
        game.fireLaser(position_, direction_, false);
        game.markDanger(game.playerPosition(), 22, false);
        game.machineGun(position_);
        game.spawnEnemyNear(position_, static_cast<EnemyKind>(2 + std::rand() % 3));
        game.airStrike();
        setShield(70);
        skillCooldown_ = 105;
        break;
    default:
        skillCooldown_ = 80;
        break;
    }
}

void EnemyTank::chooseNewDirection(Game& game, bool forcedTurn)
{
    const Direction previous = direction_;
    const Vec2 target = game.enemyAttractedTarget(position_);
    const int dx = target.x - position_.x;
    const int dy = target.y - position_.y;

    Direction primary = Direction::Down;
    Direction secondary = Direction::Right;
    if (std::abs(dx) > std::abs(dy))
    {
        primary = dx >= 0 ? Direction::Right : Direction::Left;
        secondary = dy >= 0 ? Direction::Down : Direction::Up;
    }
    else
    {
        primary = dy >= 0 ? Direction::Down : Direction::Up;
        secondary = dx >= 0 ? Direction::Right : Direction::Left;
    }

    Direction sideA = primary == Direction::Up || primary == Direction::Down ? Direction::Left : Direction::Up;
    Direction sideB = primary == Direction::Up || primary == Direction::Down ? Direction::Right : Direction::Down;
    Direction choices[4] = {primary, secondary, sideA, sideB};

    const int roll = (std::rand() + seed_ + aiCounter_) % 100;
    if (!forcedTurn)
    {
        if (roll < 62)
        {
            direction_ = choices[0];
        }
        else if (roll < 82)
        {
            direction_ = choices[1];
        }
        else
        {
            direction_ = ((seed_ + aiCounter_) % 2 == 0) ? choices[2] : choices[3];
        }
    }
    else
    {
        for (int i = 0; i < 4; ++i)
        {
            Direction candidate = choices[(i + std::rand()) % 4];
            if (candidate != previous)
            {
                direction_ = candidate;
                return;
            }
        }
    }

    if (direction_ == OppositeDirection(previous) && !forcedTurn && roll < 90)
    {
        direction_ = secondary;
    }
}

void EnemyTank::chooseTurnCooldown()
{
    int base = 100;
    int range = 130;
    if (kind_ == EnemyKind::Scout)
    {
        base = 80;
        range = 100;
    }
    else if (isBoss())
    {
        base = 110;
        range = 140;
    }

    turnCooldown_ = base + std::rand() % range;
}

int EnemyTank::fixedFireInterval() const
{
    switch (kind_)
    {
    case EnemyKind::Scout:
        return 70;
    case EnemyKind::Armor:
        return 90;
    case EnemyKind::LaserGuard:
        return 80;
    case EnemyKind::Bomber:
        return 100;
    case EnemyKind::Kamikaze:
        return 140;
    case EnemyKind::StageTwoBoss:
        return 55;
    case EnemyKind::StageThreeBoss:
        return 50;
    case EnemyKind::FinalBoss:
        return 42;
    default:
        return 85;
    }
}
