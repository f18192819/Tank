#include "WinApp.h"

#include <windowsx.h>
#include <mmsystem.h>

#include <sstream>
#include <string>
#include <vector>
#include <cstdio>

#include "Game.h"
#include "IconRenderer.h"

#pragma comment(lib, "winmm.lib")

enum class Screen
{
    Home,
    Difficulty,
    Settings,
    Credits,
    Legend,
    Playing,
    Paused
};

struct Button
{
    RECT rect;
    std::wstring text;
    int id;
};

class PixelApp
{
public:
    PixelApp()
        : hwnd_(nullptr),
          screen_(Screen::Home),
          difficulty_(Difficulty::Normal),
          soundOn_(true),
          showGrid_(true),
          showFps_(false),
          largePixels_(false),
          selectedButton_(0),
          mapCacheBitmap_(nullptr),
          mapCacheVersion_(-1),
          mapCacheTile_(0),
          mapCacheW_(0),
          mapCacheH_(0),
          fpsFrames_(0),
          fpsValue_(0),
          fpsWindowStart_(timeGetTime())
    {
        game_.setDifficulty(difficulty_);
    }

    ~PixelApp()
    {
        if (mapCacheBitmap_)
        {
            DeleteObject(mapCacheBitmap_);
        }
    }

    bool create(HINSTANCE instance, int showCommand)
    {
        WNDCLASS wc = {};
        wc.lpfnWndProc = PixelApp::WindowProc;
        wc.hInstance = instance;
        wc.lpszClassName = L"PixelTankBattleWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        RegisterClass(&wc);

        hwnd_ = CreateWindowEx(
            0,
            wc.lpszClassName,
            L"Pixel Tank Battle",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1260,
            860,
            nullptr,
            nullptr,
            instance,
            this);

        if (!hwnd_)
        {
            return false;
        }

        ShowWindow(hwnd_, showCommand);
        SetTimer(hwnd_, 1, 16, nullptr);
        return true;
    }

private:
    HWND hwnd_;
    Screen screen_;
    Difficulty difficulty_;
    bool soundOn_;
    bool showGrid_;
    bool showFps_;
    bool largePixels_;
    int selectedButton_;
    Game game_;
    std::vector<Button> buttons_;
    HBITMAP mapCacheBitmap_;
    int mapCacheVersion_;
    int mapCacheTile_;
    int mapCacheW_;
    int mapCacheH_;
    int fpsFrames_;
    int fpsValue_;
    DWORD fpsWindowStart_;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        PixelApp* app = nullptr;
        if (message == WM_NCCREATE)
        {
            CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = reinterpret_cast<PixelApp*>(create->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        else
        {
            app = reinterpret_cast<PixelApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (app)
        {
            return app->handleMessage(hwnd, message, wParam, lParam);
        }

        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    LRESULT handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_TIMER:
            if (screen_ == Screen::Playing)
            {
                game_.tick();
            }
            ++fpsFrames_;
            {
                const DWORD now = timeGetTime();
                if (now - fpsWindowStart_ >= 1000)
                {
                    fpsValue_ = fpsFrames_;
                    fpsFrames_ = 0;
                    fpsWindowStart_ = now;
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_KEYDOWN:
            handleKey(wParam, lParam);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_LBUTTONDOWN:
            handleClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_MOUSEMOVE:
            handleHover(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_PAINT:
            paint();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    void handleKey(WPARAM key, LPARAM keyState)
    {
        if (screen_ == Screen::Playing)
        {
            const bool repeated = (keyState & 0x40000000) != 0;
            if (!repeated && (key == VK_SPACE || key == 'J'))
            {
                game_.queuePlayerFire();
            }
            if (!repeated && key == 'F')
            {
                game_.queueBombShell();
            }
            if (!repeated && key == 'E')
            {
                game_.queueLaser();
            }
            if (!repeated && key == 'Q')
            {
                game_.queueShovel();
            }
            if (!repeated && key == 'T')
            {
                game_.queueTrenchToggle();
            }
            if (!repeated && key == 'G')
            {
                game_.queueMine();
            }

            if (key == 'P')
            {
                screen_ = Screen::Paused;
                return;
            }

            if (key == VK_ESCAPE)
            {
                PostQuitMessage(0);
                return;
            }

            if (game_.state() == GameState::StageCleared && key == VK_RETURN)
            {
                game_.advanceStage();
                return;
            }

            if ((game_.state() == GameState::Victory || game_.state() == GameState::Defeat) && key == VK_RETURN)
            {
                game_.setDifficulty(difficulty_);
                game_.reset();
                return;
            }
            return;
        }

        if (key == VK_ESCAPE)
        {
            if (screen_ == Screen::Home)
            {
                PostQuitMessage(0);
            }
            else if (screen_ == Screen::Paused)
            {
                PostQuitMessage(0);
            }
            else
            {
                screen_ = Screen::Home;
            }
        }

        if (key == VK_UP && !buttons_.empty())
        {
            selectedButton_ = (selectedButton_ + static_cast<int>(buttons_.size()) - 1) % static_cast<int>(buttons_.size());
        }
        else if (key == VK_DOWN && !buttons_.empty())
        {
            selectedButton_ = (selectedButton_ + 1) % static_cast<int>(buttons_.size());
        }
        else if (key == VK_RETURN && !buttons_.empty())
        {
            activateButton(buttons_[selectedButton_].id);
        }
    }

    void handleClick(int x, int y)
    {
        POINT point = {x, y};
        for (std::size_t i = 0; i < buttons_.size(); ++i)
        {
            if (PtInRect(&buttons_[i].rect, point))
            {
                selectedButton_ = static_cast<int>(i);
                activateButton(buttons_[i].id);
                break;
            }
        }
    }

    void handleHover(int x, int y)
    {
        POINT point = {x, y};
        for (std::size_t i = 0; i < buttons_.size(); ++i)
        {
            if (PtInRect(&buttons_[i].rect, point))
            {
                selectedButton_ = static_cast<int>(i);
                return;
            }
        }
    }

    void activateButton(int id)
    {
        switch (id)
        {
        case 1:
            game_.setDifficulty(difficulty_);
            game_.reset();
            screen_ = Screen::Playing;
            break;
        case 2:
            screen_ = Screen::Difficulty;
            selectedButton_ = 0;
            break;
        case 3:
            screen_ = Screen::Settings;
            selectedButton_ = 0;
            break;
        case 4:
            screen_ = Screen::Legend;
            selectedButton_ = 0;
            break;
        case 5:
            PostQuitMessage(0);
            break;
        case 10:
            difficulty_ = Difficulty::Easy;
            game_.setDifficulty(difficulty_);
            screen_ = Screen::Home;
            break;
        case 11:
            difficulty_ = Difficulty::Normal;
            game_.setDifficulty(difficulty_);
            screen_ = Screen::Home;
            break;
        case 12:
            difficulty_ = Difficulty::Hard;
            game_.setDifficulty(difficulty_);
            screen_ = Screen::Home;
            break;
        case 20:
            soundOn_ = !soundOn_;
            break;
        case 21:
            showGrid_ = !showGrid_;
            break;
        case 22:
            largePixels_ = !largePixels_;
            break;
        case 23:
            showFps_ = !showFps_;
            break;
        case 24:
            screen_ = Screen::Credits;
            selectedButton_ = 0;
            break;
        case 25:
            screen_ = Screen::Home;
            selectedButton_ = 0;
            break;
        case 26:
            screen_ = Screen::Home;
            selectedButton_ = 0;
            break;
        case 30:
            screen_ = Screen::Playing;
            break;
        case 31:
            game_.setDifficulty(difficulty_);
            game_.reset();
            screen_ = Screen::Playing;
            break;
        case 32:
            screen_ = Screen::Home;
            break;
        }
    }

    void paint()
    {
        PAINTSTRUCT paintStruct;
        HDC hdc = BeginPaint(hwnd_, &paintStruct);
        RECT client;
        GetClientRect(hwnd_, &client);

        HDC memory = CreateCompatibleDC(hdc);
        HBITMAP bitmap = CreateCompatibleBitmap(hdc, client.right, client.bottom);
        HGDIOBJ oldBitmap = SelectObject(memory, bitmap);

        fill(memory, client, RGB(0, 0, 0));
        if (screen_ != Screen::Playing && screen_ != Screen::Paused)
        {
            drawPixelBackdrop(memory, client);
        }

        buttons_.clear();
        switch (screen_)
        {
        case Screen::Home:
            drawHome(memory, client);
            break;
        case Screen::Difficulty:
            drawDifficulty(memory, client);
            break;
        case Screen::Settings:
            drawSettings(memory, client);
            break;
        case Screen::Credits:
            drawCredits(memory, client);
            break;
        case Screen::Legend:
            drawLegend(memory, client);
            break;
        case Screen::Playing:
            drawGame(memory, client);
            break;
        case Screen::Paused:
            drawGame(memory, client);
            drawPause(memory, client);
            break;
        }

        BitBlt(hdc, 0, 0, client.right, client.bottom, memory, 0, 0, SRCCOPY);
        SelectObject(memory, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memory);
        EndPaint(hwnd_, &paintStruct);
    }

    void drawHome(HDC hdc, const RECT& client)
    {
        RECT leftPanel = {48, 76, 730, 670};
        RECT rightPanel = {756, 76, client.right - 48, 670};
        RECT bottomBar = {48, 700, client.right - 48, 786};
        fill(hdc, leftPanel, RGB(18, 24, 26));
        fill(hdc, rightPanel, RGB(18, 24, 26));
        fill(hdc, bottomBar, RGB(18, 24, 26));
        frame(hdc, leftPanel, RGB(217, 198, 111), 3);
        frame(hdc, rightPanel, RGB(217, 198, 111), 3);
        frame(hdc, bottomBar, RGB(92, 114, 108), 2);

        IconRenderer::DrawTankEmblem(hdc, 90, 108, 3, RGB(226, 176, 30));
        text(hdc, 252, 108, L"BATTLE CITY", 52, RGB(244, 244, 244), true);
        text(hdc, 256, 162, L"Classic pixel tank battle", 22, RGB(190, 190, 190), false);

        RECT chip = {256, 206, 438, 244};
        fill(hdc, chip, RGB(32, 42, 44));
        frame(hdc, chip, RGB(92, 114, 108), 2);
        std::wstring difficulty = L"MODE: ";
        difficulty += difficultyName();
        text(hdc, 276, 216, difficulty, 18, RGB(232, 226, 190), true);

        addButton(hdc, 126, 286, 500, 56, L"Start Game", 1);
        addButton(hdc, 126, 356, 500, 56, L"Difficulty", 2);
        addButton(hdc, 126, 426, 500, 56, L"Settings", 3);
        addButton(hdc, 126, 496, 500, 56, L"Legend", 4);
        addButton(hdc, 126, 566, 500, 56, L"Quit", 5);

        text(hdc, 782, 108, L"MISSION BRIEFING", 30, RGB(244, 244, 244), true);
        text(hdc, 782, 146, L"Use terrain, collect items, survive waves,", 18, RGB(172, 192, 180), false);
        text(hdc, 782, 170, L"and defeat every boss tank.", 18, RGB(172, 192, 180), false);

        const std::wstring modeLine = L"Mode: " + difficultyName();
        const wchar_t* lines[] = {
            modeLine.c_str(),
            L"Levels: 4",
            L"Terrain Types: 5",
            L"Items: 5",
            L"Enemies / Bosses: 8",
            L"Goal: Defeat the final boss"
        };
        for (int i = 0; i < 6; ++i)
        {
            RECT row = {782, 228 + i * 56, rightPanel.right - 32, 270 + i * 56};
            fill(hdc, row, RGB(28, 36, 38));
            frame(hdc, row, RGB(74, 92, 86), 2);
            IconRenderer::DrawMenuPointer(hdc, 800, 238 + i * 56, 12, RGB(226, 176, 30));
            text(hdc, 826, 239 + i * 56, lines[i], 20, RGB(232, 226, 190), true);
        }

        RECT missionNote = {782, 582, rightPanel.right - 32, 650};
        fill(hdc, missionNote, RGB(28, 36, 38));
        frame(hdc, missionNote, RGB(92, 114, 108), 2);
        RECT missionText = {800, 594, missionNote.right - 16, missionNote.bottom - 12};
        textBlock(hdc, missionText,
            L"Tip: trenches, swamps, shields, bombs, lasers, and mines all matter once bosses start chaining skills.",
            16, RGB(178, 196, 186), false);

        text(hdc, 176, 724, L"MOVE: WASD / Arrow    FIRE: Space / J    PAUSE: P", 18, RGB(218, 222, 210), true);
        text(hdc, 122, 752, L"ITEMS: E Laser    F Bomb    Q Shovel    T Trench    G Mine", 18, RGB(170, 188, 178), true);
    }

    void drawDifficulty(HDC hdc, const RECT& client)
    {
        (void)client;
        text(hdc, 340, 110, L"CHOOSE DIFFICULTY", 40, RGB(232, 226, 190), true);
        text(hdc, 326, 170, L"Difficulty changes spawn rate, supplies, and enemy pressure.", 18, RGB(145, 174, 156), false);
        addButton(hdc, 360, 250, 280, 58, L"Easy", 10);
        addButton(hdc, 360, 328, 280, 58, L"Normal", 11);
        addButton(hdc, 360, 406, 280, 58, L"Hard", 12);
        text(hdc, 354, 514, L"Esc returns to the home screen.", 18, RGB(154, 169, 164), false);
    }

    void drawSettings(HDC hdc, const RECT& client)
    {
        RECT info = {716, 216, client.right - 68, 550};
        text(hdc, 398, 108, L"SETTINGS", 42, RGB(232, 226, 190), true);
        text(hdc, 272, 166, L"Presentation-oriented settings for the current build.", 18, RGB(154, 169, 164), false);
        addButton(hdc, 330, 230, 340, 58, soundOn_ ? L"Sound: On" : L"Sound: Off", 20);
        addButton(hdc, 330, 308, 340, 58, showGrid_ ? L"Grid Overlay: On" : L"Grid Overlay: Off", 21);
        addButton(hdc, 330, 386, 340, 58, largePixels_ ? L"Pixel Scale: Large" : L"Pixel Scale: Standard", 22);
#ifdef _DEBUG
        addButton(hdc, 330, 464, 340, 58, showFps_ ? L"Show FPS: On" : L"Show FPS: Off", 23);
        addButton(hdc, 330, 536, 340, 58, L"Credits", 24);
        addButton(hdc, 330, 608, 340, 58, L"Back", 25);
        text(hdc, 304, 694, L"FPS is hidden by default and only shown when enabled here.", 18, RGB(154, 169, 164), false);
#else
        addButton(hdc, 330, 464, 340, 58, L"Credits", 24);
        addButton(hdc, 330, 536, 340, 58, L"Back", 25);
        text(hdc, 324, 576, L"FPS stays hidden in the polished HUD.", 18, RGB(154, 169, 164), false);
#endif
        fill(hdc, info, RGB(20, 26, 28));
        frame(hdc, info, RGB(217, 198, 111), 3);
        text(hdc, 742, 234, L"PROJECT INFO", 26, RGB(244, 244, 244), true);
        text(hdc, 742, 280, L"Project: Pixel Tank Battle / Battle City", 18, RGB(232, 226, 190), true);
        text(hdc, 742, 314, L"Type: OOP Course Project", 18, RGB(178, 196, 186), false);
        text(hdc, 742, 348, L"Renderer: Win32 API + GDI", 18, RGB(178, 196, 186), false);
        text(hdc, 742, 382, L"Focus: unified menus, icons, HUD, and guide page", 18, RGB(178, 196, 186), false);
        text(hdc, 742, 430, L"AI-assisted development note:", 18, RGB(232, 226, 190), true);
        text(hdc, 742, 462, L"Incremental enhancement on the existing project,", 16, RGB(178, 196, 186), false);
        text(hdc, 742, 484, L"without rewriting the gameplay architecture.", 16, RGB(178, 196, 186), false);
    }

    void drawCredits(HDC hdc, const RECT& client)
    {
        RECT panel = {118, 100, client.right - 118, client.bottom - 120};
        RECT left = {panel.left + 24, panel.top + 80, panel.left + 470, panel.bottom - 92};
        RECT right = {panel.left + 500, panel.top + 80, panel.right - 24, panel.bottom - 92};
        fill(hdc, panel, RGB(18, 24, 26));
        frame(hdc, panel, RGB(217, 198, 111), 3);
        fill(hdc, left, RGB(24, 32, 34));
        fill(hdc, right, RGB(24, 32, 34));
        frame(hdc, left, RGB(92, 114, 108), 2);
        frame(hdc, right, RGB(92, 114, 108), 2);

        IconRenderer::DrawTankEmblem(hdc, panel.left + 36, panel.top + 18, 2, RGB(226, 176, 30));
        text(hdc, panel.left + 176, panel.top + 18, L"CREDITS", 40, RGB(244, 244, 244), true);
        text(hdc, panel.left + 180, panel.top + 58, L"Project origin, module summary, and AI-assisted note.", 18, RGB(168, 188, 176), false);

        text(hdc, left.left + 20, left.top + 18, L"PROJECT", 24, RGB(244, 244, 244), true);
        text(hdc, left.left + 20, left.top + 58, L"Pixel Tank Battle / Battle City", 22, RGB(232, 226, 190), true);
        text(hdc, left.left + 20, left.top + 96, L"OOP Course Project", 18, RGB(178, 196, 186), false);
        text(hdc, left.left + 20, left.top + 136, L"Core Modules", 20, RGB(244, 244, 244), true);
        text(hdc, left.left + 20, left.top + 172, L"- Game / GameMap / Tank / Bullet", 18, RGB(178, 196, 186), false);
        text(hdc, left.left + 20, left.top + 202, L"- PowerUp / Effect / WinApp", 18, RGB(178, 196, 186), false);
        text(hdc, left.left + 20, left.top + 232, L"- IconRenderer / HUD / Field Guide", 18, RGB(178, 196, 186), false);
        text(hdc, left.left + 20, left.top + 278, L"Submission Focus", 20, RGB(244, 244, 244), true);
        text(hdc, left.left + 20, left.top + 314, L"- Four levels, unified visuals, and polished UI", 18, RGB(178, 196, 186), false);
        text(hdc, left.left + 20, left.top + 344, L"- Pixel-art map, items, enemies, bosses, and HUD", 18, RGB(178, 196, 186), false);

        text(hdc, right.left + 20, right.top + 18, L"AI-ASSISTED DEVELOPMENT", 24, RGB(244, 244, 244), true);
        text(hdc, right.left + 20, right.top + 58, L"This version continues from the existing Win32/GDI", 18, RGB(178, 196, 186), false);
        text(hdc, right.left + 20, right.top + 86, L"course project instead of rewriting it from scratch.", 18, RGB(178, 196, 186), false);
        text(hdc, right.left + 20, right.top + 132, L"Optimization Highlights", 20, RGB(244, 244, 244), true);
        text(hdc, right.left + 20, right.top + 168, L"- Unified icon module for menus, map, HUD, guide", 18, RGB(178, 196, 186), false);
        text(hdc, right.left + 20, right.top + 198, L"- Reworked home menu, mission panel, and controls bar", 18, RGB(178, 196, 186), false);
        text(hdc, right.left + 20, right.top + 228, L"- Boss info strip and grouped item HUD", 18, RGB(178, 196, 186), false);
        text(hdc, right.left + 20, right.top + 274, L"Build Target", 20, RGB(244, 244, 244), true);
        text(hdc, right.left + 20, right.top + 310, L"Visual Studio 2022, Debug x64, Release x64", 18, RGB(232, 226, 190), false);
        text(hdc, right.left + 20, right.top + 356, L"Presentation Note", 20, RGB(244, 244, 244), true);
        text(hdc, right.left + 20, right.top + 392, L"The UI aims to feel like one complete game system,", 18, RGB(178, 196, 186), false);
        text(hdc, right.left + 20, right.top + 420, L"not a collection of debug screens.", 18, RGB(178, 196, 186), false);

        addButton(hdc, panel.right - 276, panel.bottom - 64, 220, 40, L"Back", 25);
    }

    void drawLegend(HDC hdc, const RECT& client)
    {
        const int pageMargin = 38;
        const int columnGap = 16;
        const int columnWidth = (client.right - pageMargin * 2 - columnGap * 2) / 3;
        const int leftColumn = pageMargin;
        const int midColumn = leftColumn + columnWidth + columnGap;
        const int rightColumn = midColumn + columnWidth + columnGap;

        fill(hdc, client, RGB(8, 12, 14));
        drawPixelBackdrop(hdc, client);

        RECT header = {pageMargin, 28, client.right - pageMargin, 122};
        fill(hdc, header, RGB(18, 24, 26));
        frame(hdc, header, RGB(217, 198, 111), 3);
        drawTankEmblem(hdc, 66, 36, 2, RGB(226, 176, 30));
        text(hdc, 208, 42, L"FIELD GUIDE", 42, RGB(244, 244, 244), true);
        text(hdc, 210, 84, L"Reference for terrain, items, enemies, and battle effects.", 18, RGB(164, 188, 170), false);

        RECT hint = {850, 46, 1182, 102};
        fill(hdc, hint, RGB(26, 34, 36));
        frame(hdc, hint, RGB(92, 114, 108), 2);
        text(hdc, 870, 58, L"Use this page to compare what you", 16, RGB(220, 224, 210), false);
        text(hdc, 870, 78, L"see in battle with each mechanic.", 16, RGB(220, 224, 210), false);

        RECT terrainPanel = {leftColumn, 148, leftColumn + columnWidth, 742};
        RECT itemPanel = {midColumn, 148, midColumn + columnWidth, 742};
        RECT enemyPanel = {rightColumn, 148, rightColumn + columnWidth, 742};

        auto drawPanel = [&](const RECT& rect, const std::wstring& title, const std::wstring& subtitle)
        {
            fill(hdc, rect, RGB(20, 26, 28));
            frame(hdc, rect, RGB(217, 198, 111), 3);
            RECT strip = {rect.left + 3, rect.top + 3, rect.right - 3, rect.top + 48};
            fill(hdc, strip, RGB(34, 44, 46));
            text(hdc, rect.left + 18, rect.top + 12, title, 24, RGB(244, 244, 244), true);
            text(hdc, rect.left + 18, rect.top + 36, subtitle, 14, RGB(160, 182, 170), false);
        };

        auto drawCard = [&](int x, int y, int w, int h)
        {
            RECT card = {x, y, x + w, y + h};
            fill(hdc, card, RGB(28, 36, 38));
            frame(hdc, card, RGB(74, 92, 86), 2);
        };

        drawPanel(terrainPanel, L"Terrain", L"Map tiles and tactical spaces");
        drawPanel(itemPanel, L"Items + FX", L"Power-ups and warning cues");
        drawPanel(enemyPanel, L"Enemies + Boss", L"Roster and signature roles");

        const int icon = 34;
        const int terrainCardX = terrainPanel.left + 18;
        const int terrainCardW = terrainPanel.right - terrainPanel.left - 36;
        const int terrainCardH = 54;
        const int terrainStartY = 214;
        const int terrainGap = 12;

        struct LegendRow
        {
            char glyph;
            const wchar_t* title;
            const wchar_t* detail;
        };

        const LegendRow terrainRows[] = {
            {'#', L"Wall", L"Blocks movement and bullets"},
            {'%', L"Wood Box", L"Player breaks it, enemy shells do not"},
            {'T', L"Trench", L"Press T here to dodge normal fire"},
            {'~', L"Swamp", L"Slows both sides heavily"},
            {'N', L"Normal Spawn", L"Fast basic enemy entry point"},
            {'E', L"Elite Spawn", L"Slower advanced enemy entry point"},
            {'B', L"Base", L"Protect it or the run ends"}
        };

        for (int i = 0; i < static_cast<int>(sizeof(terrainRows) / sizeof(terrainRows[0])); ++i)
        {
            const int y = terrainStartY + i * (terrainCardH + terrainGap);
            drawCard(terrainCardX, y, terrainCardW, terrainCardH);
            RECT pad = {terrainCardX + 10, y + 10, terrainCardX + 50, y + 44};
            fill(hdc, pad, RGB(18, 24, 25));
            drawTile(hdc, terrainCardX + 13, y + 10, icon, terrainRows[i].glyph, true);
            text(hdc, terrainCardX + 64, y + 8, terrainRows[i].title, 18, RGB(240, 232, 196), true);
            text(hdc, terrainCardX + 64, y + 31, terrainRows[i].detail, 12, RGB(176, 194, 184), false);
        }

        const int itemCardW = itemPanel.right - itemPanel.left - 28;
        const int itemCardH = 56;
        const int itemLeft = itemPanel.left + 14;
        const int itemTop = 214;
        const int itemGap = 10;
        const LegendRow itemRows[] = {
            {'S', L"Shield", L"Blocks one hit"},
            {'F', L"Blast Shell", L"3x3 impact"},
            {'R', L"Laser", L"Pierces one row or column"},
            {'Q', L"Shovel", L"Turns current tile into trench"},
            {'M', L"Mine", L"One-tile ambush blast"}
        };

        for (int i = 0; i < static_cast<int>(sizeof(itemRows) / sizeof(itemRows[0])); ++i)
        {
            const int x = itemLeft;
            const int y = itemTop + i * (itemCardH + itemGap);
            drawCard(x, y, itemCardW, itemCardH);
            RECT pad = {x + 12, y + 8, x + 52, y + 48};
            fill(hdc, pad, RGB(18, 24, 25));
            drawTile(hdc, x + 15, y + 11, 34, itemRows[i].glyph);
            text(hdc, x + 66, y + 8, itemRows[i].title, 16, RGB(240, 232, 196), true);
            text(hdc, x + 66, y + 31, itemRows[i].detail, 12, RGB(176, 194, 184), false);
        }

        RECT fxPanel = {itemPanel.left + 14, 548, itemPanel.right - 20, 712};
        fill(hdc, fxPanel, RGB(24, 32, 34));
        frame(hdc, fxPanel, RGB(86, 108, 102), 2);
        text(hdc, fxPanel.left + 18, 562, L"Combat Effects", 20, RGB(244, 244, 244), true);
        text(hdc, fxPanel.left + 18, 586, L"Warning and attack visuals used in battle.", 13, RGB(164, 188, 170), false);

        RECT fxA = {fxPanel.left + 18, 618, fxPanel.left + 114, 696};
        RECT fxB = {fxPanel.left + 124, 618, fxPanel.left + 220, 696};
        RECT fxC = {fxPanel.left + 230, 618, fxPanel.left + 326, 696};
        fill(hdc, fxA, RGB(18, 22, 24));
        fill(hdc, fxB, RGB(18, 22, 24));
        fill(hdc, fxC, RGB(18, 22, 24));
        frame(hdc, fxA, RGB(74, 92, 86), 2);
        frame(hdc, fxB, RGB(74, 92, 86), 2);
        frame(hdc, fxC, RGB(74, 92, 86), 2);

        drawLaserMuzzle(hdc, 478, 638, 20, '>', true);
        drawLaserTrace(hdc, 504, 638, 20, '-', true);
        text(hdc, 476, 668, L"Laser", 13, RGB(240, 232, 196), true);

        drawSpawnWarning(hdc, 606, 634, 20, 12);
        drawExplosionBurst(hdc, 606, 652, 12, 9, 16);
        text(hdc, 572, 668, L"Spawn / Blast", 13, RGB(240, 232, 196), true);

        drawAirStrikeWarning(hdc, 712, 634, 20, 8);
        drawExplosionBurst(hdc, 712, 652, 10, 7, 16);
        text(hdc, 688, 668, L"Airstrike", 13, RGB(240, 232, 196), true);

        struct EnemyCard
        {
            EnemyKind kind;
            char glyph;
            int tile;
            const wchar_t* title;
            const wchar_t* detail;
        };

        const EnemyCard enemyRows[] = {
            {EnemyKind::Scout, '^', 34, L"Scout", L"Quick patrol tank"},
            {EnemyKind::Armor, '>', 34, L"Armor", L"Slow tough shooter"},
            {EnemyKind::LaserGuard, '>', 34, L"LaserGuard", L"Shield + line laser"},
            {EnemyKind::Bomber, 'v', 34, L"Bomber", L"Throws four-way bombs"},
            {EnemyKind::Kamikaze, '<', 34, L"Kamikaze", L"Paths in and explodes"},
            {EnemyKind::StageTwoBoss, 'v', 38, L"Boss 2", L"Bomb summon + machine gun"},
            {EnemyKind::StageThreeBoss, '<', 38, L"Boss 3", L"Invis, reflect, summon"},
            {EnemyKind::FinalBoss, '>', 38, L"Final Boss", L"All boss skills + airstrike"}
        };

        const int enemyCardW = (enemyPanel.right - enemyPanel.left - 42) / 2;
        const int enemyCardH = 116;
        const int enemyLeft = enemyPanel.left + 18;
        const int enemyTop = 214;
        const int enemyColGap = 16;
        const int enemyRowGap = 14;

        for (int i = 0; i < static_cast<int>(sizeof(enemyRows) / sizeof(enemyRows[0])); ++i)
        {
            const int col = i % 2;
            const int row = i / 2;
            const int x = enemyLeft + col * (enemyCardW + enemyColGap);
            const int y = enemyTop + row * (enemyCardH + enemyRowGap);
            drawCard(x, y, enemyCardW, enemyCardH);
            RECT arena = {x + 12, y + 12, x + 74, y + 74};
            fill(hdc, arena, RGB(18, 22, 24));
            frame(hdc, arena, RGB(74, 92, 86), 1);
            drawEnemyTank(hdc, x + 26, y + 24, enemyRows[i].tile, enemyRows[i].kind, enemyRows[i].glyph);
            text(hdc, x + 14, y + 82, enemyRows[i].title, 16, RGB(240, 232, 196), true);
            text(hdc, x + 14, y + 102, enemyRows[i].detail, 12, RGB(176, 194, 184), false);
        }

        RECT footer = {pageMargin, 758, client.right - pageMargin, 810};
        fill(hdc, footer, RGB(18, 24, 26));
        frame(hdc, footer, RGB(92, 114, 108), 2);
        text(hdc, 62, 772, L"Tip: use this page to compare HUD, effects, and enemies during your final demo.", 15, RGB(220, 224, 210), false);
        text(hdc, 62, 790, L"Esc also returns to the home screen.", 13, RGB(154, 169, 164), false);

        addButton(hdc, footer.right - 260, 763, 220, 36, L"Back", 26);
    }

    void drawPause(HDC hdc, const RECT& client)
    {
        RECT panel = {client.right / 2 - 190, 190, client.right / 2 + 190, 462};
        fill(hdc, panel, RGB(24, 28, 30));
        frame(hdc, panel, RGB(217, 198, 111), 3);
        text(hdc, panel.left + 104, panel.top + 28, L"PAUSED", 34, RGB(232, 226, 190), true);
        addButton(hdc, panel.left + 60, panel.top + 92, 260, 48, L"Resume", 30);
        addButton(hdc, panel.left + 60, panel.top + 152, 260, 48, L"Restart", 31);
        addButton(hdc, panel.left + 60, panel.top + 212, 260, 48, L"Home", 32);
    }

    void drawGame(HDC hdc, const RECT& client)
    {
        const int tile = largePixels_ ? 30 : 26;
        const int mapWidth = GameMap::Width * tile;
        const int mapHeight = GameMap::Height * tile;
        const int startX = (client.right - mapWidth) / 2;
        const int startY = 44;

        RECT topBar = {0, 0, client.right, startY};
        fill(hdc, topBar, RGB(18, 24, 26));
        frame(hdc, topBar, RGB(92, 114, 108), 2);
        RECT mapRect = {startX - 4, startY, startX + mapWidth + 4, startY + mapHeight + 4};
        fill(hdc, mapRect, RGB(60, 72, 70));
        frame(hdc, mapRect, RGB(217, 198, 111), 2);
        RECT play = {startX, startY, startX + mapWidth, startY + mapHeight};
        fill(hdc, play, RGB(0, 0, 0));

        const int currentVersion = game_.mapVersion();
        if (mapCacheBitmap_ == nullptr || mapCacheVersion_ != currentVersion || mapCacheTile_ != tile)
        {
            rebuildMapCache(hdc, tile, mapWidth, mapHeight);
            mapCacheVersion_ = currentVersion;
            mapCacheTile_ = tile;
        }

        HDC mapDC = CreateCompatibleDC(hdc);
        HGDIOBJ oldBmp = SelectObject(mapDC, mapCacheBitmap_);
        BitBlt(hdc, startX, startY, mapWidth, mapHeight, mapDC, 0, 0, SRCCOPY);
        SelectObject(mapDC, oldBmp);
        DeleteDC(mapDC);

        if (showGrid_)
        {
            drawGridOverlay(hdc, startX, startY, tile);
        }

        drawSmoothEntities(hdc, startX, startY, tile);
        drawHud(hdc, client);
    }

    void rebuildMapCache(HDC hdc, int tile, int mapWidth, int mapHeight)
    {
        if (mapCacheBitmap_ && (mapCacheW_ != mapWidth || mapCacheH_ != mapHeight))
        {
            DeleteObject(mapCacheBitmap_);
            mapCacheBitmap_ = nullptr;
        }

        HDC mapDC = CreateCompatibleDC(hdc);
        if (!mapCacheBitmap_)
        {
            mapCacheBitmap_ = CreateCompatibleBitmap(hdc, mapWidth, mapHeight);
            mapCacheW_ = mapWidth;
            mapCacheH_ = mapHeight;
        }
        HGDIOBJ oldBmp = SelectObject(mapDC, mapCacheBitmap_);

        RECT bg = {0, 0, mapWidth, mapHeight};
        fill(mapDC, bg, RGB(0, 0, 0));

        for (int y = 0; y < GameMap::Height; ++y)
        {
            for (int x = 0; x < GameMap::Width; ++x)
            {
                drawTile(mapDC, x * tile, y * tile, tile, game_.mapGlyphAt(Vec2(x, y)), true);
            }
        }

        SelectObject(mapDC, oldBmp);
        DeleteDC(mapDC);
    }

    void drawHud(HDC hdc, const RECT& client)
    {
        RECT leftGroup = {16, 6, 568, 38};
        RECT rightGroup = {client.right - 418, 6, client.right - 16, 38};
        fill(hdc, leftGroup, RGB(26, 34, 36));
        fill(hdc, rightGroup, RGB(26, 34, 36));
        frame(hdc, leftGroup, RGB(92, 114, 108), 1);
        frame(hdc, rightGroup, RGB(92, 114, 108), 1);

        std::wstringstream scoreText;
        scoreText.fill(L'0');
        scoreText.width(6);
        scoreText << game_.score();

        text(hdc, 28, 10, L"LV " + std::to_wstring(game_.wave()), 20, RGB(244, 244, 244), true);
        text(hdc, 92, 10, L"SCORE " + scoreText.str(), 20, RGB(244, 244, 244), true);
        text(hdc, 280, 10, L"ENEMY " + std::to_wstring(game_.enemyCount()), 20, RGB(244, 244, 244), true);
        text(hdc, 404, 10, L"HP", 20, RGB(244, 244, 244), true);
        for (int i = 0; i < game_.playerLives(); ++i)
        {
            RECT pip = {442 + i * 18, 12, 454 + i * 18, 24};
            fill(hdc, pip, i == 0 ? RGB(64, 216, 104) : RGB(92, 188, 132));
            frame(hdc, pip, RGB(244, 244, 244), 1);
        }
#ifdef _DEBUG
        if (showFps_)
        {
            wchar_t fpsText[32] = {};
            _snwprintf_s(fpsText, _countof(fpsText), _TRUNCATE, L"FPS %d", fpsValue_);
            text(hdc, leftGroup.right - 90, 10, fpsText, 18, RGB(188, 214, 204), true);
        }
#endif

        const int iconY = 10;
        IconRenderer::DrawTile(hdc, client.right - 390, iconY, 20, 'S');
        text(hdc, client.right - 362, 10, game_.shieldCharges() > 0 ? L"ON" : L"OFF", 18, RGB(186, 220, 255), true);
        IconRenderer::DrawTile(hdc, client.right - 312, iconY, 20, 'F');
        text(hdc, client.right - 284, 10, L"x" + std::to_wstring(game_.bombShells()), 18, RGB(244, 232, 176), true);
        IconRenderer::DrawTile(hdc, client.right - 230, iconY, 20, 'R');
        text(hdc, client.right - 202, 10, L"x" + std::to_wstring(game_.lasers()), 18, RGB(244, 232, 176), true);
        IconRenderer::DrawTile(hdc, client.right - 148, iconY, 20, 'Q');
        text(hdc, client.right - 120, 10, L"x" + std::to_wstring(game_.shovels()), 18, RGB(244, 232, 176), true);
        IconRenderer::DrawTile(hdc, client.right - 78, iconY, 20, 'M');
        text(hdc, client.right - 50, 10, L"x" + std::to_wstring(game_.mines()), 18, RGB(244, 232, 176), true);

        if (game_.bossMaxLives() > 0)
        {
            RECT bossPanel = {418, 6, 846, 38};
            fill(hdc, bossPanel, RGB(34, 24, 24));
            frame(hdc, bossPanel, RGB(128, 86, 84), 1);
            text(hdc, 430, 10, game_.wave() == 2 ? L"BOSS 2" : (game_.wave() == 3 ? L"BOSS 3" : L"FINAL BOSS"), 18, RGB(255, 228, 168), true);
            RECT bar = {526, 14, 742, 26};
            fill(hdc, bar, RGB(58, 42, 40));
            RECT hp = bar;
            hp.right = hp.left + (bar.right - bar.left) * game_.bossLives() / game_.bossMaxLives();
            fill(hdc, hp, RGB(196, 72, 62));
            const std::string status = game_.bossStatusText();
            if (!status.empty())
            {
                std::wstring bossText(status.begin(), status.end());
                text(hdc, 756, 10, bossText, 16, RGB(255, 244, 184), true);
            }
        }

        if (game_.state() == GameState::Victory || game_.state() == GameState::Defeat)
        {
            std::wstring message = game_.state() == GameState::Victory ? L"VICTORY - Enter restart, Esc quit" : L"DEFEAT - Enter restart, Esc quit";
            text(hdc, 322, 760, message, 24, RGB(255, 255, 255), true);
        }
        else if (game_.state() == GameState::StageCleared)
        {
            text(hdc, 318, 760, L"STAGE CLEAR - Press Enter for next level", 24, RGB(255, 255, 255), true);
        }
    }

    void drawTile(HDC hdc, int x, int y, int tile, char glyph, bool terrainOnly = false)
    {
        IconRenderer::DrawTile(hdc, x, y, tile, glyph, terrainOnly);
    }

    void drawSmoothEntities(HDC hdc, int startX, int startY, int tile)
    {
        for (std::size_t i = 0; i < game_.powerUps().size(); ++i)
        {
            const PowerUp& powerUp = *game_.powerUps()[i];
            if (powerUp.isAlive())
            {
                drawTile(hdc,
                    startX + static_cast<int>((powerUp.precisePosition().x - 0.5) * tile),
                    startY + static_cast<int>((powerUp.precisePosition().y - 0.5) * tile),
                    tile,
                    powerUp.glyph());
            }
        }

        for (std::size_t i = 0; i < game_.minePositions().size(); ++i)
        {
            const Vec2 mine = game_.minePositions()[i];
            drawTile(hdc,
                startX + mine.x * tile,
                startY + mine.y * tile,
                tile,
                'M');
        }

        for (std::size_t i = 0; i < game_.effects().size(); ++i)
        {
            const TimedEffect& effect = game_.effects()[i];
            const int cx = startX + static_cast<int>((effect.position.x + 0.5) * tile);
            const int cy = startY + static_cast<int>((effect.position.y + 0.5) * tile);
            if (effect.type == EffectType::SpawnWarning)
            {
                drawSpawnWarning(hdc, cx, cy, tile, effect.ticks);
            }
            else if (effect.type == EffectType::AirStrikeWarning)
            {
                drawAirStrikeWarning(hdc, cx, cy, tile, effect.ticks);
            }
            else if (effect.type == EffectType::LaserMuzzle)
            {
                drawLaserMuzzle(hdc, cx, cy, tile, effect.glyph, effect.fromPlayer);
            }
            else if (effect.type == EffectType::LaserTrace)
            {
                drawLaserTrace(hdc, cx, cy, tile, effect.glyph, effect.fromPlayer);
            }
            else if (effect.type == EffectType::Explosion)
            {
                drawExplosionBurst(hdc, cx, cy, tile, effect.ticks, effect.totalTicks);
            }
            else
            {
                drawExplosionMark(hdc, cx, cy, tile);
            }
        }

        const PlayerTank& player = game_.player();
        if (player.isAlive())
        {
            drawTank(hdc,
                startX + static_cast<int>((player.precisePosition().x - 0.5) * tile),
                startY + static_cast<int>((player.precisePosition().y - 0.5) * tile),
                tile,
                player.isShielded() ? RGB(80, 160, 220) : RGB(43, 197, 91),
                DirectionGlyph(player.direction()));
        }

        for (std::size_t i = 0; i < game_.enemies().size(); ++i)
        {
            const EnemyTank& enemy = *game_.enemies()[i];
            if (enemy.isAlive() && enemy.isVisible())
            {
                drawEnemyTank(hdc,
                    startX + static_cast<int>((enemy.precisePosition().x - 0.5) * tile),
                    startY + static_cast<int>((enemy.precisePosition().y - 0.5) * tile),
                    tile,
                    enemy.enemyKind(),
                    DirectionGlyph(enemy.direction()));
            }
        }

        for (std::size_t i = 0; i < game_.bullets().size(); ++i)
        {
            const Bullet& bullet = *game_.bullets()[i];
            if (bullet.isAlive())
            {
                const int cx = startX + static_cast<int>(bullet.precisePosition().x * tile);
                const int cy = startY + static_cast<int>(bullet.precisePosition().y * tile);
                if (bullet.kind() == BulletKind::Blast)
                {
                    drawBlastShellBullet(hdc, cx, cy, tile, bullet.direction());
                }
                else
                {
                    RECT r = {cx - tile / 8, cy - tile / 8, cx + tile / 8 + 1, cy + tile / 8 + 1};
                    fill(hdc, r, RGB(255, 255, 255));
                }
            }
        }
    }

    void drawBrickBlock(HDC hdc, int x, int y, int tile)
    {
        IconRenderer::DrawTile(hdc, x, y, tile, '%', true);
    }

    void drawSteelBlock(HDC hdc, int x, int y, int tile)
    {
        IconRenderer::DrawTile(hdc, x, y, tile, '#', true);
    }

    void drawGrassBlock(HDC hdc, int x, int y, int tile)
    {
        IconRenderer::DrawTile(hdc, x, y, tile, '~', true);
    }

    void drawTrenchBlock(HDC hdc, int x, int y, int tile)
    {
        IconRenderer::DrawTile(hdc, x, y, tile, 'T', true);
    }

    void drawSpawnerBlock(HDC hdc, int x, int y, int tile, COLORREF color)
    {
        IconRenderer::DrawTile(hdc, x, y, tile, color == RGB(180, 80, 220) ? 'E' : 'N', true);
    }

    void drawBaseBlock(HDC hdc, int x, int y, int tile)
    {
        IconRenderer::DrawTile(hdc, x, y, tile, 'B', true);
    }

    void drawItemBlock(HDC hdc, int x, int y, int tile, char glyph)
    {
        IconRenderer::DrawTile(hdc, x, y, tile, glyph, false);
    }

    void drawExplosionMark(HDC hdc, int cx, int cy, int tile)
    {
        IconRenderer::DrawExplosionMark(hdc, cx, cy, tile);
    }

    void drawExplosionBurst(HDC hdc, int cx, int cy, int tile, int ticks, int totalTicks)
    {
        IconRenderer::DrawExplosionBurst(hdc, cx, cy, tile, ticks, totalTicks);
    }

    void drawBlastShellBullet(HDC hdc, int cx, int cy, int tile, Direction direction)
    {
        IconRenderer::DrawBlastShellBullet(hdc, cx, cy, tile, direction);
    }

    void drawSpawnWarning(HDC hdc, int cx, int cy, int tile, int ticks)
    {
        IconRenderer::DrawSpawnWarning(hdc, cx, cy, tile, ticks);
    }

    void drawAirStrikeWarning(HDC hdc, int cx, int cy, int tile, int ticks)
    {
        IconRenderer::DrawAirStrikeWarning(hdc, cx, cy, tile, ticks);
    }

    void drawLaserTrace(HDC hdc, int cx, int cy, int tile, char glyph, bool fromPlayer)
    {
        IconRenderer::DrawLaserTrace(hdc, cx, cy, tile, glyph, fromPlayer);
    }

    void drawLaserMuzzle(HDC hdc, int cx, int cy, int tile, char glyph, bool fromPlayer)
    {
        IconRenderer::DrawLaserMuzzle(hdc, cx, cy, tile, glyph, fromPlayer);
    }

    void drawTank(HDC hdc, int x, int y, int tile, COLORREF color, char glyph)
    {
        IconRenderer::DrawTank(hdc, x, y, tile, color, glyph);
    }

    void drawEnemyTank(HDC hdc, int x, int y, int tile, EnemyKind kind, char glyph)
    {
        IconRenderer::DrawEnemyTank(hdc, x, y, tile, kind, glyph);
    }

    void drawTankEmblem(HDC hdc, int x, int y, int scale, COLORREF color)
    {
        IconRenderer::DrawTankEmblem(hdc, x, y, scale, color);
    }

    void addButton(HDC hdc, int x, int y, int width, int height, const std::wstring& label, int id)
    {
        Button button = {{x, y, x + width, y + height}, label, id};
        const bool selected = static_cast<int>(buttons_.size()) == selectedButton_;
        buttons_.push_back(button);
        fill(hdc, button.rect, selected ? RGB(74, 90, 70) : RGB(28, 34, 34));
        frame(hdc, button.rect, selected ? RGB(242, 232, 188) : RGB(79, 96, 88), 3);
        if (selected)
        {
            RECT strip = {x + 6, y + 6, x + 14, y + height - 6};
            fill(hdc, strip, RGB(226, 176, 30));
            IconRenderer::DrawMenuPointer(hdc, x + 18, y + height / 2 - 8, 16, RGB(242, 232, 188));
        }
        text(hdc, x + 54, y + (height - 26) / 2 - 1, label, 22, RGB(232, 226, 190), true);
    }

    void drawPixelBackdrop(HDC hdc, const RECT& client)
    {
        for (int y = 0; y < client.bottom; y += 28)
        {
            for (int x = 0; x < client.right; x += 28)
            {
                if (((x / 28) + (y / 28)) % 2 == 0)
                {
                    RECT r = {x, y, x + 28, y + 28};
                    fill(hdc, r, RGB(12, 17, 18));
                }
            }
        }

        HPEN scanPen = CreatePen(PS_SOLID, 1, RGB(14, 18, 20));
        HGDIOBJ oldPen = SelectObject(hdc, scanPen);
        for (int y = 0; y < client.bottom; y += 6)
        {
            MoveToEx(hdc, 0, y, nullptr);
            LineTo(hdc, client.right, y);
        }
        SelectObject(hdc, oldPen);
        DeleteObject(scanPen);

        for (int i = 0; i < 18; ++i)
        {
            const int px = 36 + (i * 67) % (client.right - 72);
            const int py = 20 + (i * 43) % (client.bottom - 40);
            RECT spark = {px, py, px + 2, py + 2};
            fill(hdc, spark, (i % 3 == 0) ? RGB(42, 56, 54) : RGB(28, 38, 40));
        }
    }

    void drawGridOverlay(HDC hdc, int startX, int startY, int tile)
    {
        HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(26, 42, 40));
        HGDIOBJ oldPen = SelectObject(hdc, gridPen);
        for (int x = 0; x <= GameMap::Width; ++x)
        {
            const int px = startX + x * tile;
            MoveToEx(hdc, px, startY, nullptr);
            LineTo(hdc, px, startY + GameMap::Height * tile);
        }
        for (int y = 0; y <= GameMap::Height; ++y)
        {
            const int py = startY + y * tile;
            MoveToEx(hdc, startX, py, nullptr);
            LineTo(hdc, startX + GameMap::Width * tile, py);
        }
        SelectObject(hdc, oldPen);
        DeleteObject(gridPen);
    }

    void text(HDC hdc, int x, int y, const std::wstring& value, int size, COLORREF color, bool bold)
    {
        HFONT font = CreateFont(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
        HGDIOBJ oldFont = SelectObject(hdc, font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, color);
        TextOut(hdc, x, y, value.c_str(), static_cast<int>(value.size()));
        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    void textBlock(HDC hdc, const RECT& rect, const std::wstring& value, int size, COLORREF color, bool bold)
    {
        HFONT font = CreateFont(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
        HGDIOBJ oldFont = SelectObject(hdc, font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, color);
        RECT drawRect = rect;
        DrawTextW(hdc, value.c_str(), static_cast<int>(value.size()), &drawRect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    void fill(HDC hdc, const RECT& rect, COLORREF color)
    {
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
    }

    void frame(HDC hdc, const RECT& rect, COLORREF color, int width)
    {
        HPEN pen = CreatePen(PS_SOLID, width, color);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    COLORREF lighten(COLORREF color)
    {
        int r = GetRValue(color) + 38;
        int g = GetGValue(color) + 38;
        int b = GetBValue(color) + 38;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        return RGB(r, g, b);
    }

    COLORREF darken(COLORREF color)
    {
        int r = GetRValue(color) - 58;
        int g = GetGValue(color) - 58;
        int b = GetBValue(color) - 58;
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        return RGB(r, g, b);
    }

    std::wstring difficultyName() const
    {
        if (difficulty_ == Difficulty::Easy) return L"Easy";
        if (difficulty_ == Difficulty::Hard) return L"Hard";
        return L"Normal";
    }
};

int RunWinApp(HINSTANCE instance, int showCommand)
{
    timeBeginPeriod(1);

    wchar_t selfTestFlag[8] = {};
    if (GetEnvironmentVariableW(L"TANK_SELFTEST", selfTestFlag, 8) > 0)
    {
        Game selfTestGame;
        std::string report;
        const bool passed = selfTestGame.runSelfTest(report);

        wchar_t reportPath[MAX_PATH] = {};
        if (GetEnvironmentVariableW(L"TANK_SELFTEST_REPORT", reportPath, MAX_PATH) > 0)
        {
            HANDLE file = CreateFileW(
                reportPath,
                GENERIC_WRITE,
                0,
                nullptr,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);
            if (file != INVALID_HANDLE_VALUE)
            {
                DWORD written = 0;
                WriteFile(file, report.data(), static_cast<DWORD>(report.size()), &written, nullptr);
                CloseHandle(file);
            }
        }

        return passed ? 0 : 2;
    }

    PixelApp app;
    if (!app.create(instance, showCommand))
    {
        timeEndPeriod(1);
        return 1;
    }

    MSG message;
    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    timeEndPeriod(1);
    return static_cast<int>(message.wParam);
}
