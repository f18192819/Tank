# Tank Battle

Visual Studio 2022 C++ Win32 graphical project for an OOP course design task.
The game uses a classic Battle City style: black playfield, gray status border, orange brick walls, white steel blocks, pixel tanks, a home screen, settings screen, difficulty selection, a compact in-game HUD, and lightweight animated visual effects for menus and combat readability.

## Controls

- `W/A/S/D`: move
- `J`: fire normal shell
- `T`: enter trench protection when standing on `T`
- `F`: use bomb shell, damaging a 3x3 area
- `E`: use laser, attacking a whole row or column
- `Q`: use shovel, turning the current tile into a trench except spawners
- `G`: place a mine on the current tile
- `R`: restart
- `Esc`: pause in game, return/back in menus

## Menu And Guide

- Home screen:
  - left-side hero tank art, title, and primary buttons
  - right-side `MISSION BRIEFING` panel showing mode, level count, terrain count, item count, roster size, and victory goal
  - animated checker backdrop, pixel particles, moving scanline, title pulse, and subtle button/tank feedback
  - bottom operation strip showing movement, fire, pause, and item keys
- Field Guide:
  - unified reference page for terrain, items, effects, enemies, and bosses
  - uses the same icon drawing module as the in-game map and HUD
  - panel positions follow the window width more closely instead of relying on a single fixed coordinate set

## Screens

- Home screen: start game, difficulty, settings, legend, quit, plus mission briefing panel.
- Difficulty screen: Easy, Normal, Hard.
- Settings screen: sound toggle, grid toggle, visual effects toggle, standard/large pixel mode, and project info panel.
- Debug build settings: `Show FPS` can be toggled manually for UI inspection while staying hidden by default.
- Credits screen: project summary, module overview, and AI-assisted development note.
- Game screen: pixel map, unified tank sprites, terrain, items, boss HP bar, and grouped HUD.
  - optional grid overlay can be toggled from `Settings`

## Icon System

- `tank/IconRenderer.h` / `tank/IconRenderer.cpp`
  - centralizes terrain, item, enemy, boss, warning, explosion, laser, and emblem drawing
  - reused by:
    - home/menu visuals
    - Field Guide
    - in-game map drawing
    - HUD item icons
- style rules:
  - hard-edged pixel rectangles
  - limited palette
  - shared outlines and highlights
  - color semantics for defense, danger, elite units, and terrain

## HUD

- Top-left:
  - `LV`, `HP` life pips, zero-padded `SCORE`, remaining `ENEMY`
- Top-right:
  - shield state plus item icons and counts for bomb, laser, shovel, mine
- Boss fight:
  - dedicated boss name area
  - HP bar
  - active boss status text such as shield, invisibility, or reflect
  - subtle battlefield grid and darker scanline background for clearer pixel presentation

## Dynamic Visual Effects

- `tank/VisualEffects.h` / `tank/VisualEffects.cpp`
  - centralizes menu checker scroll, pixel particles, scanline drift, title/button pulses, and shared animation timing
  - adds dynamic terrain and item overlays without changing gameplay rules
- Menu effects:
  - slow checker offset
  - persistent low-count particles
  - scanline sweep
  - title pulse
  - selected button border pulse and pointer sway
- Map effects:
  - swamp ripple pixels
  - spawn-point pulse and imminent warning highlight
  - trench conceal accent
  - base breathing glow
  - item aura / bob
  - stronger laser, warning, explosion, and debris feedback
- Runtime switch:
  - `Settings -> Visual Effects: On/Off`
  - turning it off keeps the same gameplay with a quieter static presentation

## Movement

Tanks and bullets use continuous floating-point positions for smoother movement. The map still uses grid terrain for collision, so it keeps the classic maze layout while avoiding one-cell-at-a-time jumps.

## Requirement Coverage

- Four levels.
- Five terrain types: trench `T`, crate `%`, wall `#`, swamp `~`, enemy spawners `N/E`.
- Five power-ups: shield `S`, bomb shell `F`, laser `R`, shovel `Q`, mine `M`.
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
- `TimedEffect`: warning markers, bombs, explosions, and air strike effects.
- `IconRenderer`: shared pixel-art icon and effect drawing module.
- `Renderer`: console drawing.

Open `tank.sln` in Visual Studio and run Debug x64 or x86. The project subsystem is Windows, so it opens as a graphical window instead of a console.
