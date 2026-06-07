#include "IconRenderer.h"

namespace
{
void FillRectColor(HDC hdc, const RECT& rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void FrameRectColor(HDC hdc, const RECT& rect, COLORREF color, int width)
{
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

COLORREF Lighten(COLORREF color)
{
    int r = GetRValue(color) + 38;
    int g = GetGValue(color) + 38;
    int b = GetBValue(color) + 38;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return RGB(r, g, b);
}

COLORREF Darken(COLORREF color)
{
    int r = GetRValue(color) - 58;
    int g = GetGValue(color) - 58;
    int b = GetBValue(color) - 58;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    return RGB(r, g, b);
}

void DrawBrickBlock(HDC hdc, int x, int y, int tile)
{
    RECT base = {x, y, x + tile, y + tile};
    FillRectColor(hdc, base, RGB(122, 72, 30));
    FrameRectColor(hdc, base, RGB(70, 44, 20), 2);

    RECT lid = {x + tile / 6, y + tile / 8, x + tile * 5 / 6, y + tile / 4};
    RECT body = {x + tile / 6, y + tile / 4, x + tile * 5 / 6, y + tile * 5 / 6};
    RECT shadow = {x + tile / 5, y + tile * 5 / 6, x + tile * 4 / 5, y + tile * 6 / 7};
    FillRectColor(hdc, lid, RGB(192, 116, 56));
    FillRectColor(hdc, body, RGB(162, 94, 42));
    FillRectColor(hdc, shadow, RGB(88, 50, 24));

    RECT bandA = {x + tile / 5, y + tile / 3, x + tile / 4, y + tile * 3 / 5};
    RECT bandB = {x + tile * 3 / 4, y + tile / 3, x + tile * 4 / 5, y + tile * 3 / 5};
    FillRectColor(hdc, bandA, RGB(76, 52, 22));
    FillRectColor(hdc, bandB, RGB(76, 52, 22));
}

void DrawSteelBlock(HDC hdc, int x, int y, int tile)
{
    RECT base = {x, y, x + tile, y + tile};
    FillRectColor(hdc, base, RGB(142, 148, 152));
    FrameRectColor(hdc, base, RGB(72, 78, 84), 2);

    RECT bright = {x + tile / 6, y + tile / 6, x + tile * 5 / 6, y + tile / 3};
    RECT mid = {x + tile / 5, y + tile / 3, x + tile * 4 / 5, y + tile * 2 / 3};
    RECT shadow = {x + tile / 6, y + tile * 2 / 3, x + tile * 5 / 6, y + tile * 5 / 6};
    FillRectColor(hdc, bright, RGB(242, 242, 240));
    FillRectColor(hdc, mid, RGB(190, 194, 198));
    FillRectColor(hdc, shadow, RGB(92, 96, 100));
}

void DrawSwampBlock(HDC hdc, int x, int y, int tile)
{
    RECT base = {x, y, x + tile, y + tile};
    FillRectColor(hdc, base, RGB(44, 102, 34));

    const int unit = tile / 5 > 2 ? tile / 5 : 3;
    for (int py = 0; py < tile; py += unit)
    {
        for (int px = 0; px < tile; px += unit)
        {
            const int idx = ((px / unit) + (py / unit)) % 4;
            COLORREF color = RGB(18, 74, 20);
            if (idx == 1) color = RGB(112, 178, 34);
            else if (idx == 2) color = RGB(152, 204, 60);
            else if (idx == 3) color = RGB(60, 126, 28);
            RECT patch = {x + px, y + py, x + px + unit, y + py + unit};
            FillRectColor(hdc, patch, color);
        }
    }

    RECT puddle = {x + tile / 4, y + tile / 2, x + tile * 3 / 4, y + tile * 3 / 4};
    FillRectColor(hdc, puddle, RGB(72, 144, 48));
}

void DrawTrenchBlock(HDC hdc, int x, int y, int tile)
{
    RECT base = {x, y, x + tile, y + tile};
    FillRectColor(hdc, base, RGB(66, 52, 34));
    RECT leftBank = {x, y, x + tile / 4, y + tile};
    RECT rightBank = {x + tile * 3 / 4, y, x + tile, y + tile};
    RECT trench = {x + tile / 4, y + tile / 7, x + tile * 3 / 4, y + tile * 6 / 7};
    FillRectColor(hdc, leftBank, RGB(104, 82, 50));
    FillRectColor(hdc, rightBank, RGB(104, 82, 50));
    FillRectColor(hdc, trench, RGB(28, 22, 18));
    for (int i = 0; i < 3; ++i)
    {
        RECT plank = {x + tile / 4 + 2, y + tile / 5 + i * tile / 5, x + tile * 3 / 4 - 2, y + tile / 5 + i * tile / 5 + 3};
        FillRectColor(hdc, plank, RGB(126, 94, 58));
    }
}

void DrawSpawnerBlock(HDC hdc, int x, int y, int tile, COLORREF color)
{
    RECT outer = {x + tile / 7, y + tile / 7, x + tile * 6 / 7, y + tile * 6 / 7};
    RECT inner = {x + tile / 4, y + tile / 4, x + tile * 3 / 4, y + tile * 3 / 4};
    FrameRectColor(hdc, outer, color, 3);
    FrameRectColor(hdc, inner, RGB(255, 255, 255), 2);
    RECT core = {x + tile / 2 - 2, y + tile / 2 - 2, x + tile / 2 + 3, y + tile / 2 + 3};
    FillRectColor(hdc, core, color);
}

void DrawBaseBlock(HDC hdc, int x, int y, int tile)
{
    RECT outer = {x + 2, y + 2, x + tile - 2, y + tile - 2};
    FillRectColor(hdc, outer, RGB(214, 170, 34));
    FrameRectColor(hdc, outer, RGB(255, 244, 176), 2);

    RECT roof = {x + tile / 5, y + tile / 6, x + tile * 4 / 5, y + tile / 3};
    RECT door = {x + tile / 3, y + tile / 2, x + tile * 2 / 3, y + tile - 4};
    FillRectColor(hdc, roof, RGB(255, 226, 104));
    FillRectColor(hdc, door, RGB(114, 70, 22));
}

void DrawItemBlock(HDC hdc, int x, int y, int tile, char glyph)
{
    RECT shell = {x + 2, y + 2, x + tile - 2, y + tile - 2};
    FillRectColor(hdc, shell, RGB(16, 22, 28));
    FrameRectColor(hdc, shell, RGB(224, 228, 216), 2);

    const int cx = x + tile / 2;
    const int cy = y + tile / 2;
    switch (glyph)
    {
    case 'S':
    {
        RECT outer = {x + tile / 4, y + tile / 6, x + tile * 3 / 4, y + tile * 4 / 5};
        RECT inner = {x + tile / 3, y + tile / 4, x + tile * 2 / 3, y + tile * 11 / 16};
        FillRectColor(hdc, outer, RGB(60, 132, 255));
        FillRectColor(hdc, inner, RGB(186, 222, 255));
        FrameRectColor(hdc, outer, RGB(255, 255, 255), 2);
        break;
    }
    case 'F':
    {
        RECT shellBody = {x + tile / 3, y + tile / 5, x + tile * 2 / 3, y + tile * 4 / 5};
        RECT tip = {x + tile * 2 / 3, y + tile / 3, x + tile * 4 / 5, y + tile * 2 / 3};
        FillRectColor(hdc, shellBody, RGB(58, 58, 64));
        FrameRectColor(hdc, shellBody, RGB(255, 214, 82), 2);
        FillRectColor(hdc, tip, RGB(255, 142, 36));
        RECT band = {x + tile / 3, y + tile / 2 - 2, x + tile * 2 / 3, y + tile / 2 + 2};
        FillRectColor(hdc, band, RGB(255, 238, 126));
        break;
    }
    case 'R':
    {
        RECT emitter = {x + tile / 4, cy - tile / 7, x + tile / 2, cy + tile / 7};
        RECT beam = {x + tile / 2, cy - tile / 10, x + tile * 4 / 5, cy + tile / 10};
        FillRectColor(hdc, emitter, RGB(255, 92, 98));
        FillRectColor(hdc, beam, RGB(255, 214, 92));
        RECT spark = {x + tile * 4 / 5, cy - tile / 6, x + tile * 4 / 5 + 3, cy + tile / 6};
        FillRectColor(hdc, spark, RGB(255, 255, 255));
        break;
    }
    case 'Q':
    {
        RECT handle = {x + tile / 4, y + tile / 5, x + tile / 3, y + tile * 3 / 4};
        RECT blade = {x + tile / 3, y + tile / 2 - 2, x + tile * 3 / 4, y + tile * 3 / 4};
        FillRectColor(hdc, handle, RGB(138, 92, 54));
        FillRectColor(hdc, blade, RGB(216, 220, 226));
        FrameRectColor(hdc, blade, RGB(255, 255, 255), 1);
        RECT ring = {x + tile / 5, y + tile / 5, x + tile / 3, y + tile / 3};
        FrameRectColor(hdc, ring, RGB(216, 220, 226), 2);
        break;
    }
    case 'M':
    {
        RECT plate = {x + tile / 4, y + tile / 3, x + tile * 3 / 4, y + tile * 2 / 3};
        RECT core = {cx - tile / 10, cy - tile / 10, cx + tile / 10, cy + tile / 10};
        FillRectColor(hdc, plate, RGB(58, 66, 60));
        FrameRectColor(hdc, plate, RGB(255, 214, 82), 2);
        FillRectColor(hdc, core, RGB(220, 58, 48));
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(236, 236, 210));
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        MoveToEx(hdc, x + tile / 4, y + tile / 3, nullptr);
        LineTo(hdc, x + tile / 6, y + tile / 5);
        MoveToEx(hdc, x + tile * 3 / 4, y + tile / 3, nullptr);
        LineTo(hdc, x + tile * 5 / 6, y + tile / 5);
        MoveToEx(hdc, x + tile / 4, y + tile * 2 / 3, nullptr);
        LineTo(hdc, x + tile / 6, y + tile * 4 / 5);
        MoveToEx(hdc, x + tile * 3 / 4, y + tile * 2 / 3, nullptr);
        LineTo(hdc, x + tile * 5 / 6, y + tile * 4 / 5);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
        break;
    }
    default:
        break;
    }
}
}

namespace IconRenderer
{
void DrawTile(HDC hdc, int x, int y, int tile, char glyph, bool terrainOnly)
{
    COLORREF base = RGB(0, 0, 0);
    switch (glyph)
    {
    case '#':
        DrawSteelBlock(hdc, x, y, tile);
        return;
    case '%':
        DrawBrickBlock(hdc, x, y, tile);
        return;
    case 'T':
        DrawTrenchBlock(hdc, x, y, tile);
        return;
    case '~':
        DrawSwampBlock(hdc, x, y, tile);
        return;
    case 'N':
        DrawSpawnerBlock(hdc, x, y, tile, RGB(80, 160, 240));
        return;
    case 'E':
        DrawSpawnerBlock(hdc, x, y, tile, RGB(180, 80, 220));
        return;
    case 'B':
        DrawBaseBlock(hdc, x, y, tile);
        return;
    case 'X':
        base = RGB(180, 55, 44);
        break;
    case '*':
        base = RGB(246, 232, 142);
        break;
    case '!':
        base = RGB(218, 74, 55);
        break;
    case 'S':
    case 'F':
    case 'R':
    case 'Q':
    case 'M':
        DrawItemBlock(hdc, x, y, tile, glyph);
        return;
    case '@':
    case '^':
    case '>':
    case 'v':
    case '<':
        if (terrainOnly)
        {
            return;
        }
        DrawTank(hdc, x, y, tile, RGB(43, 197, 91), glyph);
        return;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case 'L':
    case 'C':
    case 'Z':
        if (terrainOnly)
        {
            return;
        }
        DrawTank(hdc, x, y, tile, RGB(210, 218, 224), glyph);
        return;
    default:
        break;
    }

    RECT rect = {x, y, x + tile, y + tile};
    if (glyph != ' ')
    {
        FillRectColor(hdc, rect, base);
        RECT inner = {x + tile / 4, y + tile / 4, x + tile * 3 / 4, y + tile * 3 / 4};
        FillRectColor(hdc, inner, Lighten(base));
    }
}

void DrawExplosionMark(HDC hdc, int cx, int cy, int tile)
{
    HPEN white = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
    HPEN pink = CreatePen(PS_SOLID, 2, RGB(255, 120, 220));
    HGDIOBJ oldPen = SelectObject(hdc, white);
    MoveToEx(hdc, cx - tile / 2, cy, nullptr);
    LineTo(hdc, cx + tile / 2, cy);
    MoveToEx(hdc, cx, cy - tile / 2, nullptr);
    LineTo(hdc, cx, cy + tile / 2);
    SelectObject(hdc, pink);
    MoveToEx(hdc, cx - tile / 3, cy - tile / 3, nullptr);
    LineTo(hdc, cx + tile / 3, cy + tile / 3);
    MoveToEx(hdc, cx + tile / 3, cy - tile / 3, nullptr);
    LineTo(hdc, cx - tile / 3, cy + tile / 3);
    SelectObject(hdc, oldPen);
    DeleteObject(white);
    DeleteObject(pink);
}

void DrawExplosionBurst(HDC hdc, int cx, int cy, int tile, int ticks, int totalTicks)
{
    const int age = totalTicks - ticks;
    const int outer = tile / 2 - age * tile / 24;
    const int inner = tile / 4 + age * tile / 20;
    RECT fire = {cx - outer, cy - outer, cx + outer, cy + outer};
    RECT core = {cx - inner, cy - inner, cx + inner, cy + inner};
    FillRectColor(hdc, fire, RGB(240, 76, 36));
    FillRectColor(hdc, core, RGB(255, 218, 82));

    HPEN smoke = CreatePen(PS_SOLID, 2, RGB(80, 80, 76));
    HGDIOBJ oldPen = SelectObject(hdc, smoke);
    MoveToEx(hdc, cx - outer, cy, nullptr);
    LineTo(hdc, cx + outer, cy);
    MoveToEx(hdc, cx, cy - outer, nullptr);
    LineTo(hdc, cx, cy + outer);
    SelectObject(hdc, oldPen);
    DeleteObject(smoke);
}

void DrawBlastShellBullet(HDC hdc, int cx, int cy, int tile, Direction direction)
{
    const int longRadius = tile / 5;
    const int shortRadius = tile / 9;
    RECT body = {};
    RECT flame = {};
    if (direction == Direction::Left || direction == Direction::Right)
    {
        body = {cx - longRadius, cy - shortRadius, cx + longRadius, cy + shortRadius};
        const int flameX = direction == Direction::Right ? cx - longRadius - tile / 8 : cx + longRadius;
        flame = {flameX, cy - shortRadius, flameX + tile / 8, cy + shortRadius};
    }
    else
    {
        body = {cx - shortRadius, cy - longRadius, cx + shortRadius, cy + longRadius};
        const int flameY = direction == Direction::Down ? cy - longRadius - tile / 8 : cy + longRadius;
        flame = {cx - shortRadius, flameY, cx + shortRadius, flameY + tile / 8};
    }

    FillRectColor(hdc, body, RGB(46, 46, 52));
    FrameRectColor(hdc, body, RGB(255, 210, 70), 2);
    FillRectColor(hdc, flame, RGB(255, 104, 38));
    RECT core = {cx - tile / 14, cy - tile / 14, cx + tile / 14, cy + tile / 14};
    FillRectColor(hdc, core, RGB(255, 244, 150));
}

void DrawSpawnWarning(HDC hdc, int cx, int cy, int tile, int ticks)
{
    const COLORREF color = (ticks / 3) % 2 == 0 ? RGB(255, 90, 90) : RGB(255, 230, 120);
    HPEN pen = CreatePen(PS_SOLID, 3, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Ellipse(hdc, cx - tile / 2, cy - tile / 2, cx + tile / 2, cy + tile / 2);
    Ellipse(hdc, cx - tile / 3, cy - tile / 3, cx + tile / 3, cy + tile / 3);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void DrawAirStrikeWarning(HDC hdc, int cx, int cy, int tile, int ticks)
{
    const COLORREF color = (ticks / 2) % 2 == 0 ? RGB(255, 54, 54) : RGB(255, 214, 214);
    HPEN pen = CreatePen(PS_SOLID, 3, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    MoveToEx(hdc, cx - tile / 2, cy - tile / 2, nullptr);
    LineTo(hdc, cx + tile / 2, cy - tile / 2);
    MoveToEx(hdc, cx - tile / 2, cy, nullptr);
    LineTo(hdc, cx + tile / 2, cy);
    MoveToEx(hdc, cx - tile / 2, cy + tile / 2, nullptr);
    LineTo(hdc, cx + tile / 2, cy + tile / 2);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void DrawLaserTrace(HDC hdc, int cx, int cy, int tile, char glyph, bool fromPlayer)
{
    const COLORREF color = fromPlayer ? RGB(70, 255, 255) : RGB(255, 96, 96);
    HPEN pen = CreatePen(PS_SOLID, 4, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    if (glyph == '|')
    {
        MoveToEx(hdc, cx, cy - tile / 2, nullptr);
        LineTo(hdc, cx, cy + tile / 2);
    }
    else
    {
        MoveToEx(hdc, cx - tile / 2, cy, nullptr);
        LineTo(hdc, cx + tile / 2, cy);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void DrawLaserMuzzle(HDC hdc, int cx, int cy, int tile, char glyph, bool fromPlayer)
{
    const COLORREF color = fromPlayer ? RGB(70, 255, 255) : RGB(255, 96, 96);
    HPEN pen = CreatePen(PS_SOLID, 4, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    if (glyph == '^')
    {
        MoveToEx(hdc, cx, cy, nullptr);
        LineTo(hdc, cx, cy - tile / 2);
    }
    else if (glyph == 'v')
    {
        MoveToEx(hdc, cx, cy, nullptr);
        LineTo(hdc, cx, cy + tile / 2);
    }
    else if (glyph == '<')
    {
        MoveToEx(hdc, cx, cy, nullptr);
        LineTo(hdc, cx - tile / 2, cy);
    }
    else
    {
        MoveToEx(hdc, cx, cy, nullptr);
        LineTo(hdc, cx + tile / 2, cy);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void DrawTank(HDC hdc, int x, int y, int tile, COLORREF color, char glyph)
{
    RECT leftTrack = {x + tile / 7, y + tile / 5, x + tile / 3, y + tile * 4 / 5};
    RECT rightTrack = {x + tile * 2 / 3, y + tile / 5, x + tile * 6 / 7, y + tile * 4 / 5};
    RECT body = {x + tile / 4, y + tile / 4, x + tile * 3 / 4, y + tile * 3 / 4};
    FillRectColor(hdc, leftTrack, Darken(color));
    FillRectColor(hdc, rightTrack, Darken(color));
    FillRectColor(hdc, body, color);
    FrameRectColor(hdc, leftTrack, RGB(255, 255, 255), 1);
    FrameRectColor(hdc, rightTrack, RGB(255, 255, 255), 1);
    FrameRectColor(hdc, body, RGB(255, 255, 255), 2);
    RECT turret = {x + tile * 3 / 8, y + tile * 3 / 8, x + tile * 5 / 8, y + tile * 5 / 8};
    FillRectColor(hdc, turret, Lighten(color));

    RECT barrel = {x + tile / 2 - 2, y + 1, x + tile / 2 + 3, y + tile / 2};
    if (glyph == '>')
    {
        barrel = RECT{x + tile / 2, y + tile / 2 - 2, x + tile - 1, y + tile / 2 + 3};
    }
    else if (glyph == '<')
    {
        barrel = RECT{x + 1, y + tile / 2 - 2, x + tile / 2, y + tile / 2 + 3};
    }
    else if (glyph == 'v')
    {
        barrel = RECT{x + tile / 2 - 2, y + tile / 2, x + tile / 2 + 3, y + tile - 1};
    }
    FillRectColor(hdc, barrel, RGB(255, 255, 255));
}

void DrawEnemyTank(HDC hdc, int x, int y, int tile, EnemyKind kind, char glyph)
{
    COLORREF color = RGB(210, 218, 224);
    switch (kind)
    {
    case EnemyKind::Scout:
        color = RGB(206, 220, 232);
        break;
    case EnemyKind::Armor:
        color = RGB(238, 176, 62);
        break;
    case EnemyKind::LaserGuard:
        color = RGB(88, 178, 245);
        break;
    case EnemyKind::Bomber:
        color = RGB(210, 106, 68);
        break;
    case EnemyKind::Kamikaze:
        color = RGB(228, 74, 74);
        break;
    case EnemyKind::StageTwoBoss:
        color = RGB(190, 92, 230);
        break;
    case EnemyKind::StageThreeBoss:
        color = RGB(72, 212, 170);
        break;
    case EnemyKind::FinalBoss:
        color = RGB(236, 64, 184);
        break;
    default:
        break;
    }

    DrawTank(hdc, x, y, tile, color, glyph);

    if (kind == EnemyKind::Armor)
    {
        RECT plate = {x + tile / 4, y + tile / 6, x + tile * 3 / 4, y + tile / 4};
        FillRectColor(hdc, plate, RGB(255, 238, 160));
    }
    else if (kind == EnemyKind::LaserGuard)
    {
        RECT lens = {x + tile / 2 - 3, y + tile / 2 - 3, x + tile / 2 + 4, y + tile / 2 + 4};
        FillRectColor(hdc, lens, RGB(255, 255, 255));
        FrameRectColor(hdc, lens, RGB(80, 220, 255), 2);
    }
    else if (kind == EnemyKind::Bomber)
    {
        RECT bomb = {x + tile / 6, y + tile / 6, x + tile / 3, y + tile / 3};
        FillRectColor(hdc, bomb, RGB(40, 40, 40));
        FrameRectColor(hdc, bomb, RGB(255, 230, 80), 1);
    }
    else if (kind == EnemyKind::Kamikaze)
    {
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        MoveToEx(hdc, x + tile / 4, y + tile / 4, nullptr);
        LineTo(hdc, x + tile * 3 / 4, y + tile * 3 / 4);
        MoveToEx(hdc, x + tile * 3 / 4, y + tile / 4, nullptr);
        LineTo(hdc, x + tile / 4, y + tile * 3 / 4);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
    else if (kind == EnemyKind::StageTwoBoss || kind == EnemyKind::StageThreeBoss || kind == EnemyKind::FinalBoss)
    {
        RECT crown = {x + tile / 5, y + 1, x + tile * 4 / 5, y + tile / 6};
        FillRectColor(hdc, crown, RGB(255, 235, 80));
        FrameRectColor(hdc, crown, RGB(255, 255, 255), 1);
        if (kind == EnemyKind::FinalBoss)
        {
            RECT core = {x + tile / 3, y + tile / 3, x + tile * 2 / 3, y + tile * 2 / 3};
            FrameRectColor(hdc, core, RGB(255, 255, 255), 3);
        }
    }
}

void DrawTankEmblem(HDC hdc, int x, int y, int scale, COLORREF color)
{
    RECT leftTrack = {x, y + 24 * scale, x + 10 * scale, y + 54 * scale};
    RECT rightTrack = {x + 48 * scale, y + 24 * scale, x + 58 * scale, y + 54 * scale};
    RECT hull = {x + 8 * scale, y + 24 * scale, x + 50 * scale, y + 48 * scale};
    RECT turret = {x + 18 * scale, y + 8 * scale, x + 38 * scale, y + 30 * scale};
    RECT cupola = {x + 22 * scale, y + 4 * scale, x + 30 * scale, y + 12 * scale};
    RECT barrel = {x + 36 * scale, y + 15 * scale, x + 60 * scale, y + 21 * scale};
    FillRectColor(hdc, leftTrack, Darken(color));
    FillRectColor(hdc, rightTrack, Darken(color));
    FillRectColor(hdc, hull, color);
    FillRectColor(hdc, turret, Lighten(color));
    FillRectColor(hdc, cupola, RGB(255, 226, 104));
    FillRectColor(hdc, barrel, RGB(38, 47, 42));

    RECT trim = {x + 12 * scale, y + 30 * scale, x + 46 * scale, y + 34 * scale};
    FillRectColor(hdc, trim, RGB(255, 240, 176));
    FrameRectColor(hdc, hull, RGB(255, 244, 176), 2);
    FrameRectColor(hdc, turret, RGB(255, 244, 176), 2);
}

void DrawMenuPointer(HDC hdc, int x, int y, int size, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, CreateSolidBrush(color));
    POINT points[3] = {
        {x, y + size / 2},
        {x + size, y},
        {x + size, y + size}
    };
    Polygon(hdc, points, 3);
    DeleteObject(SelectObject(hdc, oldBrush));
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}
}
