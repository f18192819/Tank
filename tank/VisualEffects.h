#pragma once

#include <windows.h>

#include <vector>

#include "Effect.h"
#include "Types.h"

class VisualEffects
{
public:
    VisualEffects();

    void setEnabled(bool enabled);
    bool enabled() const;
    void update(double deltaSeconds, const RECT& client);

    void drawMenuBackdrop(HDC hdc, const RECT& client) const;
    COLORREF titleColor() const;
    COLORREF selectedButtonBorderColor() const;
    COLORREF selectedButtonFillColor() const;
    COLORREF footerPromptColor() const;
    int pointerOffset() const;
    bool shouldPulseEmblem() const;

    void drawTileOverlay(HDC hdc, int x, int y, int tile, char glyph, bool highlightTrench, bool threatenedBase, bool imminentSpawn) const;
    void drawPowerUpAura(HDC hdc, int x, int y, int tile, char glyph) const;
    void drawTimedEffectOverlay(HDC hdc, int cx, int cy, int tile, const TimedEffect& effect) const;
    int itemBobOffset(const Vec2& cell, char glyph) const;

private:
    struct PixelParticle
    {
        float x;
        float y;
        float dx;
        float dy;
        float brightness;
        float phase;
    };

    bool enabled_;
    double timeSeconds_;
    double checkerOffsetX_;
    double checkerOffsetY_;
    double scanlineY_;
    int viewportW_;
    int viewportH_;
    std::vector<PixelParticle> particles_;

    void ensureParticles(const RECT& client);
    static COLORREF lerpColor(COLORREF from, COLORREF to, double t);
    double pulse01(double frequency, double phase = 0.0) const;
};
