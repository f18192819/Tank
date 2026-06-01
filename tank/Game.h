#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Bullet.h"
#include "Effect.h"
#include "GameMap.h"
#include "PowerUp.h"
#include "Tank.h"

class Renderer;

class Game
{
public:
    struct PendingSpawn
    {
        Vec2 center;
        EnemyKind kind;
        int ticks;

        PendingSpawn(const Vec2& spawnCenter, EnemyKind enemyKind, int delayTicks)
            : center(spawnCenter), kind(enemyKind), ticks(delayTicks)
        {
        }
    };

    Game();

    void run();
    void tick();
    void reset();
    void advanceStage();
    void setDifficulty(Difficulty difficulty);
    Difficulty difficulty() const;
    bool canOccupy(const Vec2& position, const Entity* requester) const;
    bool canOccupyPrecise(const FloatVec2& position, const Entity* requester) const;
    bool spawnBullet(const Vec2& position, Direction direction, bool fromPlayer, int speed = 1);
    bool spawnBullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed = 1);
    bool resolveBulletHit(const Bullet& bullet, const Vec2& target);
    bool buildTrench(const Vec2& position);
    bool isSlowed(const Vec2& position) const;
    bool tryEnterTrench();
    void explodeArea(const Vec2& center, bool fromPlayer);
    void fireLaser(const Vec2& start, Direction direction, bool fromPlayer);
    void markDanger(const Vec2& center, int ticks, bool fromPlayer);
    void throwBomb(const Vec2& from, const Vec2& toward, bool fromPlayer, int range, int warningTicks);
    void placeDecoy(const Vec2& position);
    void machineGun(const Vec2& center);
    void airStrike();
    void spawnEnemyNear(const Vec2& center, EnemyKind kind);
    Vec2 randomAround(const Vec2& center, int radius) const;
    Vec2 enemyAttractedTarget(const Vec2& enemyPosition) const;
    bool hasLineOfSight(const Vec2& from, const Vec2& to) const;
    std::vector<Vec2> findPath(const Vec2& from, const Vec2& to) const;
    Vec2 playerPosition() const;
    int playerLives() const;
    int score() const;
    int enemyCount() const;
    int wave() const;
    int bossLives() const;
    int bossMaxLives() const;
    int bombShells() const;
    int lasers() const;
    int shovels() const;
    int decoys() const;
    int shieldCharges() const;
    GameState state() const;
    int pendingSpawnCount() const;
    std::string bossStatusText() const;
    bool runSelfTest(std::string& report);
    void draw(std::vector<std::string>& buffer) const;
    const std::vector<std::unique_ptr<EnemyTank> >& enemies() const;
    const std::vector<std::unique_ptr<Bullet> >& bullets() const;
    const std::vector<std::unique_ptr<PowerUp> >& powerUps() const;
    const std::vector<TimedEffect>& effects() const;
    const PlayerTank& player() const;
    char mapGlyphAt(const Vec2& position) const;

private:
    GameMap map_;
    PlayerTank player_;
    std::vector<std::unique_ptr<EnemyTank> > enemies_;
    std::vector<std::unique_ptr<Bullet> > bullets_;
    std::vector<std::unique_ptr<PowerUp> > powerUps_;
    std::vector<TimedEffect> effects_;
    std::vector<PendingSpawn> pendingSpawns_;
    GameState state_;
    int score_;
    int wave_;
    int spawnTick_;
    int powerUpTick_;
    int playerMoveSkip_;
    int decoyTicks_;
    Vec2 decoyPosition_;
    Difficulty difficulty_;
    int frameAccumulator_;
    bool playerInTrench_;

    void update();
    void handleGlobalInput();
    void spawnWave();
    void spawnRandomEnemy(bool elite);
    void spawnPowerUp();
    void updatePendingSpawns();
    void updateEffects();
    void collectPowerUps();
    void removeDeadEntities();
    bool isOccupiedByTank(const Vec2& position, const Entity* requester) const;
    void damageTankAt(const Vec2& position, bool fromPlayer, DamageType damageType, Direction attackDirection);
    void checkGameState();
};
