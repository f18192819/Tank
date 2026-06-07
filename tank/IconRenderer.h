#pragma once

#include <windows.h>

#include "Types.h"

namespace IconRenderer
{
void DrawTile(HDC hdc, int x, int y, int tile, char glyph, bool terrainOnly = false);
void DrawTank(HDC hdc, int x, int y, int tile, COLORREF color, char glyph);
void DrawEnemyTank(HDC hdc, int x, int y, int tile, EnemyKind kind, char glyph);
void DrawExplosionMark(HDC hdc, int cx, int cy, int tile);
void DrawExplosionBurst(HDC hdc, int cx, int cy, int tile, int ticks, int totalTicks);
void DrawBlastShellBullet(HDC hdc, int cx, int cy, int tile, Direction direction);
void DrawSpawnWarning(HDC hdc, int cx, int cy, int tile, int ticks);
void DrawAirStrikeWarning(HDC hdc, int cx, int cy, int tile, int ticks);
void DrawLaserTrace(HDC hdc, int cx, int cy, int tile, char glyph, bool fromPlayer);
void DrawLaserMuzzle(HDC hdc, int cx, int cy, int tile, char glyph, bool fromPlayer);
void DrawTankEmblem(HDC hdc, int x, int y, int scale, COLORREF color);
void DrawMenuPointer(HDC hdc, int x, int y, int size, COLORREF color);
}
