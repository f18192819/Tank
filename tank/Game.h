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
    bool spawnBullet(const FloatVec2& position, Direction direction, bool fromPlayer, int speed, BulletKind kind);
    bool spawnBlastShell(const FloatVec2& position, Direction direction, bool fromPlayer, int speed = 1);
    bool resolveBulletHit(const Bullet& bullet, const Vec2& target);
    bool resolveBulletHit(const Bullet& bullet, const FloatVec2& preciseTarget);
    void queuePlayerFire();
    bool consumePlayerFireRequest();
    void queueBombShell();
    bool consumeBombShellRequest();
    void queueLaser();
    bool consumeLaserRequest();
    void queueShovel();
    bool consumeShovelRequest();
    void queueTrenchToggle();
    bool consumeTrenchRequest();
    void queueMine();
    bool consumeMineRequest();
    bool buildTrench(const Vec2& position);
    bool isSlowed(const Vec2& position) const;
    bool tryEnterTrench();
    void explodeArea(const Vec2& center, bool fromPlayer);
    void fireLaser(const Vec2& start, Direction direction, bool fromPlayer);
    void fireLaser(const FloatVec2& start, Direction direction, bool fromPlayer);
    void markDanger(const Vec2& center, int ticks, bool fromPlayer);
    void throwBomb(const Vec2& from, const Vec2& toward, bool fromPlayer, int range, int warningTicks);
    bool placeMine(const Vec2& position);
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
    int mines() const;
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
    const std::vector<Vec2>& minePositions() const;
    const PlayerTank& player() const;
    char mapGlyphAt(const Vec2& position) const;
    int mapVersion() const;

private:
    GameMap map_;
    PlayerTank player_;
    std::vector<std::unique_ptr<EnemyTank> > enemies_;
    std::vector<std::unique_ptr<Bullet> > bullets_;
    std::vector<std::unique_ptr<PowerUp> > powerUps_;
    std::vector<TimedEffect> effects_;
    std::vector<PendingSpawn> pendingSpawns_;
    std::vector<Vec2> mines_;
    GameState state_;
    int score_;
    int wave_;
    int spawnTick_;
    int powerUpTick_;
    int playerMoveSkip_;
    Difficulty difficulty_;
    int frameAccumulator_;
    bool playerInTrench_;
    bool playerFireQueued_;
    bool bombShellQueued_;
    bool laserQueued_;
    bool shovelQueued_;
    bool trenchQueued_;
    bool mineQueued_;

    void update();
    void handleGlobalInput();
    void spawnWave();
    void spawnRandomEnemy(bool elite);
    void spawnPowerUp();
    void updatePendingSpawns();
    void updateEffects();
    void collectPowerUps();
    void triggerMines();
    void resolveBulletCollisions();
    void removeDeadEntities();
    void createExplosionEffect(const Vec2& center, bool fromPlayer);
    bool findBlastTankHit(const Bullet& bullet, const FloatVec2& preciseTarget, Vec2& hitCenter) const;
    bool isOccupiedByTank(const Vec2& position, const Entity* requester) const;
    bool isOccupiedByEnemyTank(const Vec2& position, bool fromPlayer) const;
    void damageTankAt(const Vec2& position, bool fromPlayer, DamageType damageType, Direction attackDirection);
    void checkGameState();
};
