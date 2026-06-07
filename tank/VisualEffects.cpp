#include "VisualEffects.h"

#include <algorithm>
#include <cmath>

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

int ClampByte(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

int MaxInt(int a, int b)
{
    return a > b ? a : b;
}

int MinInt(int a, int b)
{
    return a < b ? a : b;
}

double ClampDouble(double value, double low, double high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}
}

VisualEffects::VisualEffects()
    : enabled_(true),
      timeSeconds_(0.0),
      checkerOffsetX_(0.0),
      checkerOffsetY_(0.0),
      scanlineY_(0.0),
      viewportW_(0),
      viewportH_(0)
{
}

void VisualEffects::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

bool VisualEffects::enabled() const
{
    return enabled_;
}

void VisualEffects::update(double deltaSeconds, const RECT& client)
{
    viewportW_ = client.right - client.left;
    viewportH_ = client.bottom - client.top;
    ensureParticles(client);

    if (deltaSeconds < 0.0)
    {
        deltaSeconds = 0.0;
    }
    if (deltaSeconds > 0.05)
    {
        deltaSeconds = 0.05;
    }

    timeSeconds_ += deltaSeconds;
    if (!enabled_)
    {
        return;
    }

    checkerOffsetX_ += deltaSeconds * 6.0;
    checkerOffsetY_ += deltaSeconds * 2.0;
    scanlineY_ += deltaSeconds * 34.0;

    const double checkerSize = 28.0;
    while (checkerOffsetX_ >= checkerSize)
    {
        checkerOffsetX_ -= checkerSize;
    }
    while (checkerOffsetY_ >= checkerSize)
    {
        checkerOffsetY_ -= checkerSize;
    }
    while (scanlineY_ >= viewportH_ + 24.0)
    {
        scanlineY_ -= viewportH_ + 24.0;
    }

    for (std::size_t i = 0; i < particles_.size(); ++i)
    {
        PixelParticle& particle = particles_[i];
        particle.x += particle.dx * static_cast<float>(deltaSeconds);
        particle.y += particle.dy * static_cast<float>(deltaSeconds);
        particle.phase += static_cast<float>(deltaSeconds * 0.9);
        if (particle.x < 0.0f) particle.x += static_cast<float>(viewportW_);
        if (particle.x >= viewportW_) particle.x -= static_cast<float>(viewportW_);
        if (particle.y < 0.0f) particle.y += static_cast<float>(viewportH_);
        if (particle.y >= viewportH_) particle.y -= static_cast<float>(viewportH_);
    }
}

void VisualEffects::drawMenuBackdrop(HDC hdc, const RECT& client) const
{
    const int checker = 28;
    const int offsetX = enabled_ ? static_cast<int>(checkerOffsetX_) : 0;
    const int offsetY = enabled_ ? static_cast<int>(checkerOffsetY_) : 0;
    for (int y = -checker; y < client.bottom + checker; y += checker)
    {
        for (int x = -checker; x < client.right + checker; x += checker)
        {
            RECT tile = {x - offsetX, y - offsetY, x - offsetX + checker, y - offsetY + checker};
            FillRectColor(hdc, tile, (((x / checker) + (y / checker)) & 1) == 0 ? RGB(10, 14, 16) : RGB(14, 20, 22));
        }
    }

    for (int y = 0; y < client.bottom; y += 6)
    {
        const int phaseShift = enabled_ ? static_cast<int>(scanlineY_) : 0;
        const COLORREF lineColor = ((y + phaseShift) / 12) % 2 == 0 ? RGB(16, 22, 24) : RGB(12, 18, 20);
        RECT line = {0, y, client.right, y + 1};
        FillRectColor(hdc, line, lineColor);
    }

    if (enabled_)
    {
        const int scanY = static_cast<int>(scanlineY_) - 10;
        RECT scanBand = {0, scanY, client.right, scanY + 18};
        if (scanBand.bottom > 0 && scanBand.top < client.bottom)
        {
            FillRectColor(hdc, scanBand, RGB(18, 32, 28));
        }

        for (std::size_t i = 0; i < particles_.size(); ++i)
        {
            const PixelParticle& particle = particles_[i];
            const double sparkle = 0.45 + 0.55 * std::sin(timeSeconds_ * 1.8 + particle.phase);
            const int brightness = ClampByte(static_cast<int>(particle.brightness * sparkle));
            const int px = static_cast<int>(particle.x);
            const int py = static_cast<int>(particle.y);
            COLORREF starColor = RGB(brightness / 3, brightness / 2, brightness);
            if (i % 9 == 0)
            {
                starColor = RGB(brightness / 2, brightness / 2, brightness);
            }
            else if (i % 13 == 0)
            {
                starColor = RGB(brightness, brightness * 3 / 4, brightness / 2);
            }

            RECT core = {px, py, px + 2, py + 2};
            FillRectColor(hdc, core, starColor);

            if ((i % 7 == 0 && sparkle > 0.82) || (i % 11 == 0 && sparkle > 0.72))
            {
                RECT h = {px - 2, py, px + 4, py + 1};
                RECT v = {px, py - 2, px + 1, py + 4};
                FillRectColor(hdc, h, starColor);
                FillRectColor(hdc, v, starColor);
            }

            if (i % 17 == 0 && sparkle > 0.88)
            {
                RECT glowA = {px - 1, py - 1, px + 3, py + 3};
                RECT glowB = {px + 3, py + 1, px + 8, py + 2};
                FillRectColor(hdc, glowA, RGB(brightness / 4, brightness / 3, brightness / 2));
                FillRectColor(hdc, glowB, RGB(brightness / 5, brightness / 4, brightness / 3));
            }
        }

        for (int i = 0; i < 3; ++i)
        {
            const int trailX = 120 + ((static_cast<int>(timeSeconds_ * (16 + i * 3)) + i * 180) % MaxInt(180, client.right - 220));
            const int trailY = 74 + i * 116;
            RECT star = {trailX, trailY, trailX + 3, trailY + 3};
            RECT tail = {trailX - 14, trailY + 1, trailX, trailY + 2};
            FillRectColor(hdc, star, RGB(210, 224, 255));
            FillRectColor(hdc, tail, RGB(66, 82, 112));
        }
    }
    else
    {
        for (int i = 0; i < 18; ++i)
        {
            const int px = 36 + (i * 67) % (client.right - 72);
            const int py = 20 + (i * 43) % (client.bottom - 40);
            RECT spark = {px, py, px + 2, py + 2};
            FillRectColor(hdc, spark, (i % 3 == 0) ? RGB(42, 56, 54) : RGB(28, 38, 40));
        }
    }
}

COLORREF VisualEffects::titleColor() const
{
    if (!enabled_)
    {
        return RGB(244, 244, 244);
    }
    return lerpColor(RGB(226, 222, 196), RGB(255, 248, 216), pulse01(0.45));
}

COLORREF VisualEffects::selectedButtonBorderColor() const
{
    if (!enabled_)
    {
        return RGB(242, 232, 188);
    }
    return lerpColor(RGB(198, 186, 124), RGB(255, 240, 180), pulse01(1.3));
}

COLORREF VisualEffects::selectedButtonFillColor() const
{
    if (!enabled_)
    {
        return RGB(74, 90, 70);
    }
    return lerpColor(RGB(68, 84, 68), RGB(86, 100, 78), pulse01(1.0, 0.2));
}

COLORREF VisualEffects::footerPromptColor() const
{
    if (!enabled_)
    {
        return RGB(218, 222, 210);
    }
    return lerpColor(RGB(164, 180, 168), RGB(230, 234, 214), pulse01(0.55));
}

int VisualEffects::pointerOffset() const
{
    if (!enabled_)
    {
        return 0;
    }
    return static_cast<int>(std::sin(timeSeconds_ * 5.0) * 3.0);
}

bool VisualEffects::shouldPulseEmblem() const
{
    return enabled_ && pulse01(1.2, 0.4) > 0.62;
}

void VisualEffects::drawTileOverlay(HDC hdc, int x, int y, int tile, char glyph, bool highlightTrench, bool threatenedBase, bool imminentSpawn) const
{
    if (!enabled_)
    {
        return;
    }

    const int tick = static_cast<int>(timeSeconds_ * 8.0);
    switch (glyph)
    {
    case '~':
    {
        for (int i = 0; i < 4; ++i)
        {
            const int px = x + ((tick + i * 5 + x / 7 + y / 9) % (tile - 6)) + 2;
            const int py = y + ((tick * 2 + i * 7 + x / 11) % (tile - 6)) + 2;
            RECT ripple = {px, py, px + 4, py + 2};
            FillRectColor(hdc, ripple, i % 2 == 0 ? RGB(146, 206, 72) : RGB(86, 166, 52));
        }
        break;
    }
    case 'N':
    case 'E':
    {
        const COLORREF pulseColor = glyph == 'N' ? RGB(90, 182, 255) : RGB(208, 112, 255);
        const COLORREF ringColor = imminentSpawn ? RGB(255, 92, 92) : pulseColor;
        const int inset = 2 + static_cast<int>(pulse01(1.8, (x + y) * 0.03) * 4.0);
        RECT ring = {x + inset, y + inset, x + tile - inset, y + tile - inset};
        FrameRectColor(hdc, ring, ringColor, imminentSpawn ? 3 : 2);
        break;
    }
    case 'T':
    {
        RECT shade = {x + 2, y + tile - 6, x + tile - 2, y + tile - 2};
        FillRectColor(hdc, shade, RGB(18, 18, 18));
        if (highlightTrench)
        {
            RECT left = {x + 1, y + 3, x + 4, y + tile - 3};
            RECT right = {x + tile - 4, y + 3, x + tile - 1, y + tile - 3};
            FillRectColor(hdc, left, RGB(78, 110, 96));
            FillRectColor(hdc, right, RGB(78, 110, 96));
        }
        break;
    }
    case '#':
    {
        const int px = x + 3 + (tick + x / 8 + y / 8) % MaxInt(4, tile - 10);
        RECT glint = {px, y + 4, px + 5, y + 6};
        FillRectColor(hdc, glint, RGB(244, 244, 244));
        break;
    }
    case 'B':
    case 'X':
    {
        const COLORREF glowColor = threatenedBase || glyph == 'X'
            ? lerpColor(RGB(112, 22, 18), RGB(244, 70, 54), pulse01(2.8))
            : lerpColor(RGB(112, 84, 12), RGB(246, 206, 82), pulse01(0.8));
        RECT glow = {x - 2, y - 2, x + tile + 2, y + tile + 2};
        FrameRectColor(hdc, glow, glowColor, 2);
        break;
    }
    default:
        break;
    }
}

void VisualEffects::drawPowerUpAura(HDC hdc, int x, int y, int tile, char glyph) const
{
    if (!enabled_)
    {
        return;
    }

    COLORREF aura = RGB(184, 184, 184);
    switch (glyph)
    {
    case 'S':
        aura = lerpColor(RGB(86, 154, 255), RGB(200, 230, 255), pulse01(1.5, x * 0.02));
        break;
    case 'F':
        aura = lerpColor(RGB(220, 84, 42), RGB(255, 214, 82), pulse01(1.7, x * 0.03));
        break;
    case 'R':
        aura = lerpColor(RGB(82, 255, 255), RGB(200, 255, 255), pulse01(1.8, y * 0.04));
        break;
    case 'Q':
        aura = lerpColor(RGB(184, 176, 112), RGB(248, 226, 144), pulse01(1.1, y * 0.03));
        break;
    case 'M':
        aura = lerpColor(RGB(220, 58, 48), RGB(255, 216, 92), pulse01(2.1, x * 0.02 + y * 0.01));
        break;
    default:
        break;
    }

    RECT ring = {x - 2, y - 2, x + tile + 2, y + tile + 2};
    FrameRectColor(hdc, ring, aura, 2);
    if (glyph == 'M')
    {
        RECT dot = {x + tile / 2 - 2, y + 4, x + tile / 2 + 2, y + 8};
        FillRectColor(hdc, dot, pulse01(3.0) > 0.5 ? RGB(255, 82, 82) : RGB(80, 24, 24));
    }
}

void VisualEffects::drawTimedEffectOverlay(HDC hdc, int cx, int cy, int tile, const TimedEffect& effect) const
{
    if (!enabled_)
    {
        return;
    }

    if (effect.type == EffectType::Explosion)
    {
        const int age = effect.totalTicks - effect.ticks;
        const int frame = MinInt(3, age * 4 / MaxInt(1, effect.totalTicks));
        if (frame == 0)
        {
            RECT core = {cx - 3, cy - 3, cx + 4, cy + 4};
            FillRectColor(hdc, core, RGB(255, 244, 152));
        }
        else if (frame == 1)
        {
            RECT h = {cx - tile / 3, cy - 2, cx + tile / 3, cy + 2};
            RECT v = {cx - 2, cy - tile / 3, cx + 2, cy + tile / 3};
            FillRectColor(hdc, h, RGB(255, 210, 96));
            FillRectColor(hdc, v, RGB(255, 210, 96));
        }
        else if (frame == 2)
        {
            RECT wave = {cx - tile / 2, cy - tile / 2, cx + tile / 2, cy + tile / 2};
            FrameRectColor(hdc, wave, RGB(255, 152, 74), 2);
        }
        else
        {
            for (int i = 0; i < 4; ++i)
            {
                RECT debris = {
                    cx - tile / 3 + i * 4,
                    cy - tile / 4 + ((i % 2 == 0) ? -2 : 4),
                    cx - tile / 3 + i * 4 + 3,
                    cy - tile / 4 + ((i % 2 == 0) ? -2 : 4) + 3
                };
                FillRectColor(hdc, debris, RGB(104, 88, 62));
            }
        }
        return;
    }

    if (effect.type == EffectType::Debris)
    {
        for (int i = 0; i < 5; ++i)
        {
            RECT chip = {
                cx - tile / 3 + i * 4,
                cy - tile / 4 + ((i % 2 == 0) ? -4 : 5),
                cx - tile / 3 + i * 4 + 3,
                cy - tile / 4 + ((i % 2 == 0) ? -4 : 5) + 3
            };
            FillRectColor(hdc, chip, i % 2 == 0 ? RGB(170, 104, 56) : RGB(102, 62, 28));
        }
        return;
    }

    if (effect.type == EffectType::LaserTrace || effect.type == EffectType::LaserMuzzle)
    {
        HPEN glow = CreatePen(PS_SOLID, 6, effect.fromPlayer ? RGB(74, 214, 214) : RGB(214, 86, 86));
        HGDIOBJ oldPen = SelectObject(hdc, glow);
        if (effect.glyph == '|' || effect.glyph == '^' || effect.glyph == 'v')
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
        DeleteObject(glow);
        return;
    }

    if (effect.type == EffectType::SpawnWarning)
    {
        RECT alarm = {cx - tile / 2, cy - tile / 2, cx + tile / 2, cy + tile / 2};
        FrameRectColor(hdc, alarm, effect.ticks < 8 ? RGB(255, 92, 92) : RGB(255, 220, 128), 2);
        return;
    }

    if (effect.type == EffectType::AirStrikeWarning)
    {
        const COLORREF color = effect.ticks < 8 ? RGB(255, 82, 82) : RGB(255, 176, 176);
        RECT flash = {cx - tile / 2, cy - 3, cx + tile / 2, cy + 3};
        FillRectColor(hdc, flash, color);
        return;
    }

    if (effect.type == EffectType::Warning)
    {
        RECT countdown = {cx - tile / 2 + 2, cy - tile / 2 + 2, cx + tile / 2 - 2, cy + tile / 2 - 2};
        FrameRectColor(hdc, countdown, effect.ticks < 7 ? RGB(255, 94, 94) : RGB(244, 224, 128), 2);
    }
}

int VisualEffects::itemBobOffset(const Vec2& cell, char glyph) const
{
    if (!enabled_)
    {
        return 0;
    }

    const double phase = cell.x * 0.37 + cell.y * 0.22 + static_cast<int>(glyph) * 0.03;
    return static_cast<int>(std::sin(timeSeconds_ * 2.2 + phase) * 2.0);
}

void VisualEffects::ensureParticles(const RECT& client)
{
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (!particles_.empty() && width == viewportW_ && height == viewportH_)
    {
        return;
    }

    particles_.clear();
    const int count = 132;
    particles_.reserve(count);
    unsigned int seed = static_cast<unsigned int>((width + 13) * (height + 29) + 97);
    for (int i = 0; i < count; ++i)
    {
        seed = seed * 1103515245u + 12345u;
        const float px = static_cast<float>((seed >> 8) % MaxInt(1, width));
        seed = seed * 1103515245u + 12345u;
        const float py = static_cast<float>((seed >> 8) % MaxInt(1, height));
        seed = seed * 1103515245u + 12345u;
        const float speed = 3.0f + static_cast<float>((seed >> 8) % 14) * 0.2f;
        seed = seed * 1103515245u + 12345u;
        const float bright = 96.0f + static_cast<float>((seed >> 8) % 120);
        seed = seed * 1103515245u + 12345u;
        const float phase = static_cast<float>((seed >> 8) % 628) / 100.0f;

        PixelParticle particle = {};
        particle.x = px;
        particle.y = py;
        particle.dx = (i % 5 == 0) ? 0.0f : ((i % 2 == 0) ? 5.0f : -3.0f);
        particle.dy = 0.8f + speed * 0.1f;
        particle.brightness = bright;
        particle.phase = phase;
        particles_.push_back(particle);
    }
}

COLORREF VisualEffects::lerpColor(COLORREF from, COLORREF to, double t)
{
    const double clamped = ClampDouble(t, 0.0, 1.0);
    const int r = ClampByte(static_cast<int>(GetRValue(from) + (GetRValue(to) - GetRValue(from)) * clamped));
    const int g = ClampByte(static_cast<int>(GetGValue(from) + (GetGValue(to) - GetGValue(from)) * clamped));
    const int b = ClampByte(static_cast<int>(GetBValue(from) + (GetBValue(to) - GetBValue(from)) * clamped));
    return RGB(r, g, b);
}

double VisualEffects::pulse01(double frequency, double phase) const
{
    return 0.5 + 0.5 * std::sin(timeSeconds_ * frequency * 6.283185307179586 + phase);
}
