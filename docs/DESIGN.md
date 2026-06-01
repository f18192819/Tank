# 设计说明

## 1. 项目简介

本项目基于当前目录中已有的 Win32/GDI 坦克大战课程工程继续增量开发，保留原有窗口、像素地图、坦克、子弹、道具和菜单框架，并在现有架构上扩展四关卡、五种地形、五种道具、基础敌人、高级敌人和 Boss 技能系统。

## 2. 总体架构

- `WinApp`
  - Win32 窗口创建、消息循环、菜单/暂停/绘制入口。
- `Game`
  - 游戏主循环状态、关卡推进、碰撞、伤害、道具拾取、全局技能调度。
- `GameMap`
  - 地图数据、地形规则、四关布局、视野与 BFS 路径辅助。
- `Entity`
  - 所有游戏对象的公共基类，负责位置、存活状态和精确坐标。
- `Tank` / `PlayerTank` / `EnemyTank`
  - 坦克层次结构，负责移动、射击、AI、Boss 技能与特殊状态。
- `Bullet`
  - 连续坐标子弹移动与命中判定入口。
- `PowerUp`
  - 道具实体与地图刷新对象。
- `TimedEffect`
  - 预警线、爆炸、刷新提示、热诱弹、空袭提示等时序特效。

## 3. 类设计说明

- `Entity`
  - 封装 `position_`、`precisePosition_`、`alive_`。
- `Tank`
  - 抽象坦克的生命、朝向、射速、子弹速度和移动速度。
- `PlayerTank`
  - 负责键盘控制、库存类道具消耗与玩家行动。
- `EnemyTank`
  - 负责普通敌人、高级敌人与 Boss 的差异化行为。
- `GameMap`
  - 将地图行为从 `Game` 中拆开，负责地形规则和网格工具函数。
- `Game`
  - 作为系统编排层，不直接承担底层绘制。

## 4. 面向对象特性体现

- 继承
  - `Entity -> Tank -> PlayerTank / EnemyTank`
- 多态
  - `update()` 与 `glyph()` 由实体子类覆写。
- 封装
  - 地图、血量、冷却、效果等状态通过类方法维护。
- 组合
  - `Game` 组合 `GameMap`、玩家、敌人、子弹、道具与特效容器。

## 5. 地形系统设计

- `Trench`
  - 玩家可切换战壕状态；仅对敌方普通子弹提供规避效果。
- `WoodenBox`
  - 阻挡移动，可被玩家攻击或特殊爆炸破坏。
- `Wall`
  - 不可破坏，阻挡移动与射线。
- `Swamp`
  - 降低玩家和敌人的移动节奏。
- `SpawnPoint`
  - 区分普通与精英刷新点，并保留刷新预警特效。

## 6. 敌人系统设计

- 基础敌人
  - `BasicEnemyA/Scout`：血低、移动较快、简单巡逻。
  - `BasicEnemyB/Armor`：血量更高，看到目标后更偏向定向推进。
- 高级敌人
  - `LaserShieldEnemy/LaserGuard`：技能触发时刷新护盾，并释放一条行列激光；若能看见玩家则优先朝玩家方向发射。
  - `BombThrowEnemy/Bomber`：定点投弹预警。
  - `SuicideEnemy/Kamikaze`：BFS 追踪、蓄力预警与接近 2 秒后的自爆。
- Boss
  - `BossLevel2`：炸弹召唤、机枪扫射。
  - `BossLevel3`：隐身、反弹、召唤高级敌人。
  - `FinalBoss`：激光、炸弹、机枪、召唤、反弹、空袭。

## 7. Boss 技能系统设计

- 当前实现以 `EnemyTank::useSkill()` 为统一入口。
- 结合 `skillCooldown_`、`invisibilityTicks_`、`reflectTicks_` 控制持续时间与冷却。
- `TimedEffect` 承担预警可视化与延迟触发。
- `spawnEnemyNear()` 增加了场上数量上限，用于限制 Boss 召唤和刷新点刷怪堆积。
- `Game::bossStatusText()` 将护盾、隐身、反弹等 Boss 状态暴露给 HUD，便于运行时观察。
- 最终 Boss 当前已包含激光、炸弹、机枪、召唤、反弹、空袭、护盾等多种组合技能。

## 8. 道具系统设计

- `Shield`
  - 给玩家增加一次/一段时间的护盾保护。
- `BlastShell`
  - 触发 3x3 爆炸伤害。
- `Laser`
  - 沿当前朝向直线攻击。
- `Shovel`
  - 将当前位置改造成战壕。
- `Decoy`
  - 在玩家当前格子放置热诱弹，改变可视范围内敌方目标选择，并诱导同排同列可视敌人朝该格开火。

## 9. 碰撞检测设计

- 地形碰撞
  - `GameMap::isPassable()` 与 `Game::canOccupyPrecise()` 共同控制。
- 子弹碰撞
  - `Bullet::update()` 调用 `Game::resolveBulletHit()`。
- 范围伤害
  - 通过 `explodeArea()` 与 `TimedEffect` 延时触发。
- 激光/空袭
  - 由 `fireLaser()` 与 `airStrike()` 负责。
- 激光可视化
  - `fireLaser()` 同时创建 `LaserTrace` 特效，Win32 HUD 中以不同颜色绘制玩家/敌方激光弹道。

## 10. 关卡设计

- `GameMap` 内置四套关卡布局。
- `Game::wave_` 同时承担当前关卡推进编号。
- 每关刷新组合不同，并逐步引入高级敌人与 Boss。
- 第 1 至第 3 关清场后进入 `StageCleared`，由 `Enter` 进入下一关。
- 刷新点使用 `PendingSpawn` 延迟生成，避免即时无提示刷怪。

## 11. 游戏主循环设计

- Win32 `WM_TIMER` 驱动 `game_.tick()`。
- `Game::update()` 顺序处理：
  - 玩家
  - 道具拾取
  - 敌人
  - 子弹
  - 特效
  - 待生成敌人队列
  - 刷新与关卡判定

## 12. 可扩展性分析

- 已把关卡地图、敌人类型、技能类型、伤害类型抽象为枚举和独立接口。
- 后续可以继续把 Boss 技能和投弹/空袭逻辑抽成独立 `Skill` 类。
- 目前 Boss 特殊状态仍集中在 `EnemyTank` 中，后续仍有进一步分层空间。
