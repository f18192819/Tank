#pragma once

#include <vector>

#include "Entity.h"

class Tank : public Entity
{
public:
    Tank(const Vec2& position, Direction direction, int fireCooldownTicks);

    Direction direction() const;
    EnemyKind enemyKind() const;
    int lives() const;
    int maxLives() const;
    int bulletSpeed() const;
    double moveSpeedValue() const;
    bool isShielded() const;
    void setStats(int lives, int fireCooldownTicks, int bulletSpeed, int moveInterval, EnemyKind kind);
    void setShield(int ticks);
    void resetFireCooldown();
    void reduceFireCooldown();
    bool canFire() const;
    bool takeHit();
    virtual bool isVisible() const;
    char glyph() const override;

protected:
    bool tryMove(Game& game, Direction direction);
    bool tryFire(Game& game, bool fromPlayer);

    Direction direction_;
    int fireCooldown_;
    int fireCooldownTicks_;
    int lives_;
    int maxLives_;
    int bulletSpeed_;
    int moveInterval_;
    int shieldTicks_;
    EnemyKind kind_;
    double moveSpeed_;
};

class PlayerTank : public Tank
{
public:
    explicit PlayerTank(const Vec2& position);

    void addShield(int ticks);
    void addBombShell();
    void addLaser();
    void addShovel();
    void addMine();
    int shields() const;
    int bombShells() const;
    int lasers() const;
    int shovels() const;
    int mines() const;
    void update(Game& game) override;

private:
    int bombShells_;
    int lasers_;
    int shovels_;
    int mines_;
};

class EnemyTank : public Tank
{
public:
    EnemyTank(const Vec2& position, EnemyKind kind, int seed);

    void update(Game& game) override;
    char glyph() const override;
    int scoreValue() const;
    bool isBoss() const;
    bool isVisible() const override;
    bool shouldReflectAttack(Direction attackDirection, DamageType damageType) const;
    bool reflectActive() const;
    bool invisibleActive() const;

private:
    int aiCounter_;
    int seed_;
    int skillCooldown_;
    int moveDebt_;
    int turnCooldown_;
    int fireTimer_;
    int invisibilityTicks_;
    int reflectTicks_;
    int chargeTicks_;
    int pathRefreshTicks_;
    Vec2 moveTarget_;
    std::vector<Vec2> cachedPath_;

    void useSkill(Game& game);
    void chooseNewDirection(Game& game, bool forcedTurn);
    void chooseTurnCooldown();
    int fixedFireInterval() const;
    void updateSuicidePath(Game& game);
    bool isPatrolUnit() const;
    Vec2 choosePatrolGoal(Game& game) const;
    bool followPathToward(Game& game, const Vec2& target, int refreshDelay);
};
