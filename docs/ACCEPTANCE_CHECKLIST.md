# 验收清单

| 课程/项目要求 | 实现方式 | 对应文件/类 | 测试方法 | 是否完成 |
| --- | --- | --- | --- | --- |
| 使用 C++ | 现有工程全量使用 C++ | `tank/*.cpp`, `tank/*.h` | VS2022 编译 | 是 |
| 使用 Win32 API + GDI | 保留并扩展当前窗口与 GDI 绘制 | `tank/WinApp.cpp` | 启动窗口、人工查看 | 是 |
| 基于现有项目增量开发 | 在原有工程文件内持续修改 | `tank.sln`, `tank/tank.vcxproj` | 查看工程结构 | 是 |
| Debug x64 编译通过 | MSBuild 构建 | `tank.sln` | `Debug|x64` 构建 | 是 |
| Release x64 编译通过 | MSBuild 构建 | `tank.sln` | `Release|x64` 构建 | 是 |
| 游戏可以正常启动窗口 | 生成 Win32 图形可执行文件并做进程级启动验证 | `tank/main.cpp`, `tank/WinApp.cpp` | 启动 Debug/Release 可执行文件并检查进程存活 | 是 |
| 基本输入链路可运行 | 通过脚本发送开始、移动、开火、道具、暂停与退出按键 | `tank/WinApp.cpp`, `tank/Tank.cpp`, `tank/Game.cpp` | 交互烟雾测试并检查进程存活 | 是 |
| 四个关卡 | 四套地图布局 + 波次推进 | `tank/GameMap.cpp`, `tank/Game.cpp` | 内建自检 + 运行时切关 | 是 |
| 五种地形 | 战壕/木箱/墙/沼泽/刷新点 | `tank/GameMap.*` | 内建自检 + 人工运行测试 | 是 |
| 五种道具 | 护盾/爆破弹/激光/铲子/地雷 | `tank/PowerUp.*`, `tank/Tank.cpp`, `tank/Game.cpp` | 内建自检覆盖拾取、护盾消耗、激光轨迹、爆炸破坏、战壕建造、地雷触发 | 是 |
| 至少 8 种敌人/Boss | 两类基础敌人、三类高级敌人、三类 Boss | `tank/Tank.cpp`, `tank/Types.h` | 内建自检 + 代码检查 | 是 |
| 不同生命/移速/弹速 | `setStats` 区分参数 | `tank/Tank.cpp` | 运行时观察与代码检查 | 是 |
| 高级敌人特殊行为 | 激光/投弹/自爆寻路 | `tank/Tank.cpp`, `tank/GameMap.cpp` | 内建自检覆盖 LaserGuard、Bomber、Kamikaze 行为 | 是 |
| Boss 技能系统 | 冷却 + 特效 + 技能调用 | `tank/Tank.cpp`, `tank/Game.cpp`, `tank/Effect.h` | 内建自检覆盖二关 Boss、三关 Boss、最终 Boss 技能触发 | 是 |
| 胜利/失败/重开 | `GameState` 与 WinApp 控制 | `tank/Game.cpp`, `tank/WinApp.cpp` | 内建自检覆盖关卡推进/胜负/重置，交互烟雾测试覆盖 `Enter`/`Esc` 链路 | 是 |
| 主菜单布局更平衡 | 左主视觉、右侧 `MISSION BRIEFING`、底部操作条 | `tank/WinApp.cpp` | 运行程序人工查看 | 是 |
| 菜单按钮选中效果更明显 | 亮边框、左侧色条、像素指针、悬停同步 | `tank/WinApp.cpp`, `tank/IconRenderer.cpp` | 键盘/鼠标交互测试 | 是 |
| 图标系统统一 | 单独 `IconRenderer` 模块复用地形/道具/敌人/效果绘制 | `tank/IconRenderer.*`, `tank/WinApp.cpp` | 运行程序观察主菜单、Field Guide、HUD、地图 | 是 |
| Settings / Credits 基本可展示 | `Settings` 提供可调项，`Credits` 展示项目信息与 AI 说明 | `tank/WinApp.cpp` | 运行程序人工查看 | 是 |
| Field Guide 风格统一 | 共用图标模块、统一标题/分栏/Back 按钮，并改为相对布局 | `tank/WinApp.cpp`, `tank/IconRenderer.*` | 运行程序人工查看 | 是 |
| HUD 更像正式界面 | 分组顶栏、隐藏默认 FPS、道具图标库存、Boss 区域、生命格 | `tank/WinApp.cpp`, `tank/IconRenderer.*` | 运行程序人工查看 | 是 |
| 主菜单具有低调动态背景 | 动态棋盘格、粒子、扫描线、标题呼吸、按钮脉冲 | `tank/VisualEffects.*`, `tank/WinApp.cpp` | 运行程序人工查看 + `Debug|x64` 构建 | 是 |
| 地图具有轻量动态效果 | 沼泽水纹、刷新点脉冲、基地呼吸、道具光晕、木箱碎屑、激光/爆炸增强 | `tank/VisualEffects.*`, `tank/Game.cpp`, `tank/Effect.h`, `tank/WinApp.cpp` | 运行程序人工查看 + 构建验证 | 是 |
| Visual Effects 开关真实生效 | `Settings -> Visual Effects: On/Off` 控制统一动态层 | `tank/WinApp.cpp`, `tank/VisualEffects.*` | 运行程序切换设置 + 构建验证 | 是 |
| 设置项真实生效 | `Grid Overlay` 与 `Show FPS (Debug only)` 均接入实际绘制逻辑 | `tank/WinApp.cpp` | 运行程序切换设置 + 构建验证 | 是 |
| 文档补齐 | 设计/测试/AI/验收文档 | `docs/*` | 文档检查 | 是 |
