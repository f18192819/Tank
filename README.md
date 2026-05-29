# Tank Battle

Visual Studio 2022 C++ Win32 graphical project for an OOP course design task.
The game uses a classic Battle City style: black playfield, gray status border, orange brick walls, white steel blocks, pixel tanks, a home screen, settings screen, difficulty selection, and a compact in-game HUD.

## Controls

- `W/A/S/D`: move
- `J`: fire normal shell
- `T`: enter trench protection when standing on `T`
- `F`: use bomb shell, damaging a 3x3 area
- `E`: use laser, attacking a whole row or column
- `Q`: use shovel, turning the current tile into a trench except spawners
- `G`: place heat decoy for about 15 seconds
- `R`: restart
- `Esc`: pause in game, return/back in menus

## Screens

- Home screen: start game, difficulty, settings, quit.
- Difficulty screen: Easy, Normal, Hard.
- Settings screen: sound toggle, grid toggle, standard/large pixel mode.
- Game screen: pixel map, tank sprites, terrain, items, boss HP bar, compact HUD.

## Movement

Tanks and bullets use continuous floating-point positions for smoother movement. The map still uses grid terrain for collision, so it keeps the classic maze layout while avoiding one-cell-at-a-time jumps.

## Requirement Coverage

- Four levels.
- Five terrain types: trench `T`, crate `%`, wall `#`, swamp `~`, enemy spawners `N/E`.
- Five power-ups: shield `S`, bomb shell `F`, laser `R`, shovel `Q`, heat decoy `H`.
- Eight enemy entries: two basic enemies, three advanced enemies, and three bosses.
- Advanced enemies have special skills.
- Bosses have two or more skills; the final boss combines laser, bomb summon, machine gun, summon, shield/reflect-like protection, and air strike.

## Class Design

- `Game`: main loop, level configuration, collision, win/lose state, skills.
- `GameMap`: terrain data and terrain behavior.
- `Entity`: common position/alive base class.
- `Tank`, `PlayerTank`, `EnemyTank`: tank hierarchy with different stats and AI.
- `Bullet`: projectile movement and speed.
- `PowerUp`: collectible items.
- `TimedEffect`: warning markers, bombs, decoy, air strike effects.
- `Renderer`: console drawing.

Open `tank.sln` in Visual Studio and run Debug x64 or x86. The project subsystem is Windows, so it opens as a graphical window instead of a console.
