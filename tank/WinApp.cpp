#include "WinApp.h"

#include <windowsx.h>
#include <mmsystem.h>

#include <sstream>
#include <string>
#include <vector>

#include "Game.h"

#pragma comment(lib, "winmm.lib")

enum class Screen
{
    Home,
    Difficulty,
    Settings,
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
          largePixels_(false),
          selectedButton_(0),
          mapCacheBitmap_(nullptr),
          mapCacheVersion_(-1),
          mapCacheTile_(0),
          mapCacheW_(0),
          mapCacheH_(0)
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
    bool largePixels_;
    int selectedButton_;
    Game game_;
    std::vector<Button> buttons_;
    HBITMAP mapCacheBitmap_;
    int mapCacheVersion_;
    int mapCacheTile_;
    int mapCacheW_;
    int mapCacheH_;

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
        drawTankEmblem(hdc, 112, 88, 4, RGB(226, 176, 30));
        text(hdc, 300, 92, L"BATTLE CITY", 54, RGB(244, 244, 244), true);
        text(hdc, 304, 154, L"Classic pixel tank battle", 22, RGB(190, 190, 190), false);

        std::wstring difficulty = L"Difficulty: ";
        difficulty += difficultyName();
        text(hdc, 332, 198, difficulty, 22, RGB(226, 176, 30), false);

        addButton(hdc, 360, 260, 280, 54, L"Start Game", 1);
        addButton(hdc, 360, 328, 280, 54, L"Difficulty", 2);
        addButton(hdc, 360, 396, 280, 54, L"Settings", 3);
        addButton(hdc, 360, 464, 280, 54, L"Quit", 4);

        text(hdc, 246, 610, L"WASD move  Arrow keys move  Space/J fire  F bomb  E laser", 18, RGB(154, 169, 164), false);
        text(hdc, 234, 638, L"Q shovel  T trench  G mine  P pause  Enter continue/restart  Esc exits", 18, RGB(154, 169, 164), false);
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
        (void)client;
        text(hdc, 398, 108, L"SETTINGS", 42, RGB(232, 226, 190), true);
        addButton(hdc, 330, 230, 340, 58, soundOn_ ? L"Sound: On" : L"Sound: Off", 20);
        addButton(hdc, 330, 308, 340, 58, showGrid_ ? L"Grid: On" : L"Grid: Off", 21);
        addButton(hdc, 330, 386, 340, 58, largePixels_ ? L"Pixels: Large" : L"Pixels: Standard", 22);
        addButton(hdc, 330, 492, 340, 58, L"Back", 23);
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
        const int startY = 34;

        RECT topBar = {0, 0, client.right, startY};
        fill(hdc, topBar, RGB(150, 150, 150));
        text(hdc, 8, 3, L"PS: 99.9", 25, RGB(255, 255, 255), true);
        RECT mapRect = {startX - 4, startY, startX + mapWidth + 4, startY + mapHeight + 4};
        fill(hdc, mapRect, RGB(150, 150, 150));
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
        std::wstringstream stream;
        stream << L"LV " << game_.wave()
            << L"   HP " << game_.playerLives()
            << L"   SCORE " << game_.score()
            << L"   ENEMY " << game_.enemyCount();
        text(hdc, 172, 4, stream.str(), 22, RGB(255, 255, 255), true);

        std::wstringstream items;
        items << L"S " << (game_.shieldCharges() > 0 ? L"ON" : L"OFF")
            << L"  F x" << game_.bombShells()
            << L"  E x" << game_.lasers()
            << L"  Q x" << game_.shovels()
            << L"  G x" << game_.mines();
        text(hdc, client.right - 405, 5, items.str(), 20, RGB(255, 255, 255), false);

        if (game_.bossMaxLives() > 0)
        {
            RECT bar = {424, 25, 702, 32};
            fill(hdc, bar, RGB(58, 42, 40));
            RECT hp = bar;
            hp.right = hp.left + (bar.right - bar.left) * game_.bossLives() / game_.bossMaxLives();
            fill(hdc, hp, RGB(196, 72, 62));
            const std::string status = game_.bossStatusText();
            if (!status.empty())
            {
                std::wstring bossText(status.begin(), status.end());
                text(hdc, 716, 17, bossText, 18, RGB(255, 244, 184), true);
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
        COLORREF base = RGB(0, 0, 0);
        switch (glyph)
        {
        case '#':
            drawSteelBlock(hdc, x, y, tile);
            return;
        case '%':
            drawBrickBlock(hdc, x, y, tile);
            return;
        case 'T':
            drawTrenchBlock(hdc, x, y, tile);
            return;
        case '~':
            drawGrassBlock(hdc, x, y, tile);
            return;
        case 'N':
            drawSpawnerBlock(hdc, x, y, tile, RGB(80, 160, 240));
            return;
        case 'E':
            drawSpawnerBlock(hdc, x, y, tile, RGB(180, 80, 220));
            return;
        case 'B':
            drawBaseBlock(hdc, x, y, tile);
            return;
        case 'X': base = RGB(180, 55, 44); break;
        case '*': base = RGB(246, 232, 142); break;
        case '!': base = RGB(218, 74, 55); break;
        case 'S': case 'F': case 'R': case 'Q': case 'M':
            drawItemBlock(hdc, x, y, tile, glyph);
            return;
        case '@': case '^': case '>': case 'v': case '<':
            if (terrainOnly) return;
            drawTank(hdc, x, y, tile, RGB(43, 197, 91), glyph);
            return;
        case '1': case '2': case '3': case '4': case '5': case 'L': case 'C': case 'Z':
            if (terrainOnly) return;
            drawTank(hdc, x, y, tile, RGB(210, 218, 224), glyph);
            return;
        default:
            break;
        }

        RECT rect = {x, y, x + tile, y + tile};
        if (glyph != ' ')
        {
            fill(hdc, rect, base);
        }
        if (glyph != ' ')
        {
            RECT inner = {x + tile / 4, y + tile / 4, x + tile * 3 / 4, y + tile * 3 / 4};
            fill(hdc, inner, lighten(base));
        }
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
        RECT base = {x, y, x + tile, y + tile};
        fill(hdc, base, RGB(160, 74, 16));
        const int rowH = tile / 4;
        for (int row = 0; row < 4; ++row)
        {
            const int y0 = y + row * rowH;
            RECT mortar = {x, y0, x + tile, y0 + 2};
            fill(hdc, mortar, RGB(92, 92, 92));
            const int offset = (row % 2) ? tile / 2 : 0;
            for (int bx = -offset; bx < tile; bx += tile / 2)
            {
                RECT seam = {x + bx, y0, x + bx + 2, y0 + rowH};
                fill(hdc, seam, RGB(92, 92, 92));
            }
            RECT shine = {x + 3, y0 + 3, x + tile - 3, y0 + 5};
            fill(hdc, shine, RGB(223, 117, 30));
            RECT shadow = {x + 3, y0 + rowH - 3, x + tile - 3, y0 + rowH - 1};
            fill(hdc, shadow, RGB(113, 54, 12));
        }
    }

    void drawSteelBlock(HDC hdc, int x, int y, int tile)
    {
        RECT base = {x, y, x + tile, y + tile};
        fill(hdc, base, RGB(166, 166, 166));
        frame(hdc, base, RGB(94, 94, 94), 2);
        RECT bright = {x + tile / 6, y + tile / 7, x + tile * 5 / 6, y + tile * 3 / 7};
        fill(hdc, bright, RGB(250, 250, 250));
        RECT mid = {x + tile / 5, y + tile * 3 / 7, x + tile * 4 / 5, y + tile * 5 / 7};
        fill(hdc, mid, RGB(218, 218, 218));
        RECT shadow = {x + tile / 6, y + tile * 5 / 7, x + tile * 5 / 6, y + tile * 6 / 7};
        fill(hdc, shadow, RGB(112, 112, 112));
    }

    void drawGrassBlock(HDC hdc, int x, int y, int tile)
    {
        RECT base = {x, y, x + tile, y + tile};
        fill(hdc, base, RGB(88, 206, 30));
        const int unit = tile / 5 > 2 ? tile / 5 : 3;
        for (int py = 0; py < tile; py += unit)
        {
            for (int px = 0; px < tile; px += unit)
            {
                COLORREF color = ((px + py) / unit) % 3 == 0 ? RGB(16, 98, 18) : (((px + py) / unit) % 3 == 1 ? RGB(173, 235, 56) : RGB(48, 150, 24));
                RECT leaf = {x + px, y + py, x + px + unit / 2 + 2, y + py + unit / 2 + 2};
                fill(hdc, leaf, color);
            }
        }
    }

    void drawTrenchBlock(HDC hdc, int x, int y, int tile)
    {
        RECT base = {x, y, x + tile, y + tile};
        fill(hdc, base, RGB(45, 35, 28));
        for (int i = 0; i < 4; ++i)
        {
            RECT stripe = {x + 2, y + i * tile / 4 + 2, x + tile - 2, y + i * tile / 4 + 4};
            fill(hdc, stripe, RGB(92, 70, 42));
        }
    }

    void drawSpawnerBlock(HDC hdc, int x, int y, int tile, COLORREF color)
    {
        RECT r = {x + tile / 5, y + tile / 5, x + tile * 4 / 5, y + tile * 4 / 5};
        frame(hdc, r, color, 3);
        RECT core = {x + tile / 2 - 2, y + tile / 2 - 2, x + tile / 2 + 3, y + tile / 2 + 3};
        fill(hdc, core, RGB(255, 255, 255));
    }

    void drawBaseBlock(HDC hdc, int x, int y, int tile)
    {
        RECT base = {x + 2, y + 2, x + tile - 2, y + tile - 2};
        fill(hdc, base, RGB(218, 170, 30));
        frame(hdc, base, RGB(255, 255, 255), 2);
        RECT door = {x + tile / 3, y + tile / 2, x + tile * 2 / 3, y + tile - 3};
        fill(hdc, door, RGB(92, 55, 14));
    }

    void drawItemBlock(HDC hdc, int x, int y, int tile, char glyph)
    {
        RECT r = {x + 2, y + 2, x + tile - 2, y + tile - 2};
        fill(hdc, r, RGB(20, 28, 42));
        frame(hdc, r, RGB(235, 235, 235), 2);

        const int cx = x + tile / 2;
        const int cy = y + tile / 2;

        switch (glyph)
        {
        case 'S':
        {
            RECT outer = {x + tile / 4, y + tile / 6, x + tile * 3 / 4, y + tile * 4 / 5};
            RECT inner = {x + tile / 3, y + tile / 4, x + tile * 2 / 3, y + tile * 11 / 16};
            fill(hdc, outer, RGB(64, 148, 255));
            fill(hdc, inner, RGB(176, 224, 255));
            frame(hdc, outer, RGB(255, 255, 255), 2);
            RECT crest = {cx - tile / 10, y + tile / 5, cx + tile / 10, y + tile / 3};
            fill(hdc, crest, RGB(255, 255, 255));
            break;
        }
        case 'F':
        {
            RECT bomb = {x + tile / 4, y + tile / 4, x + tile * 3 / 4, y + tile * 3 / 4};
            RECT fuse = {cx - 1, y + tile / 7, cx + 2, y + tile / 4};
            fill(hdc, bomb, RGB(52, 52, 58));
            frame(hdc, bomb, RGB(255, 214, 82), 2);
            fill(hdc, fuse, RGB(222, 170, 72));
            RECT sparkA = {x + tile * 3 / 4 - 2, y + tile / 8, x + tile * 3 / 4 + 2, y + tile / 8 + 4};
            RECT sparkB = {x + tile * 3 / 4 + 1, y + tile / 6, x + tile * 3 / 4 + 5, y + tile / 6 + 4};
            fill(hdc, sparkA, RGB(255, 240, 120));
            fill(hdc, sparkB, RGB(255, 132, 48));
            break;
        }
        case 'R':
        {
            RECT core = {cx - tile / 10, y + tile / 5, cx + tile / 10, y + tile * 4 / 5};
            fill(hdc, core, RGB(255, 80, 92));
            RECT beam = {x + tile / 4, cy - tile / 10, x + tile * 3 / 4, cy + tile / 10};
            fill(hdc, beam, RGB(255, 214, 92));
            RECT tip = {x + tile * 3 / 4, cy - tile / 6, x + tile * 4 / 5, cy + tile / 6};
            fill(hdc, tip, RGB(255, 255, 255));
            break;
        }
        case 'Q':
        {
            RECT handle = {x + tile / 4, y + tile / 4, x + tile / 3, y + tile * 3 / 4};
            RECT blade = {x + tile / 3, y + tile / 2 - 2, x + tile * 3 / 4, y + tile * 3 / 4};
            fill(hdc, handle, RGB(136, 90, 52));
            fill(hdc, blade, RGB(214, 220, 224));
            frame(hdc, blade, RGB(255, 255, 255), 1);
            RECT cap = {x + tile / 5, y + tile / 5, x + tile / 3, y + tile / 3};
            frame(hdc, cap, RGB(214, 220, 224), 2);
            break;
        }
        case 'M':
        {
            RECT plate = {x + tile / 4, y + tile / 3, x + tile * 3 / 4, y + tile * 2 / 3};
            RECT button = {cx - tile / 10, cy - tile / 10, cx + tile / 10, cy + tile / 10};
            fill(hdc, plate, RGB(58, 64, 58));
            frame(hdc, plate, RGB(255, 214, 82), 2);
            fill(hdc, button, RGB(220, 58, 48));
            HPEN prong = CreatePen(PS_SOLID, 2, RGB(235, 235, 210));
            HGDIOBJ oldPen = SelectObject(hdc, prong);
            MoveToEx(hdc, x + tile / 4, y + tile / 3, nullptr);
            LineTo(hdc, x + tile / 6, y + tile / 5);
            MoveToEx(hdc, x + tile * 3 / 4, y + tile / 3, nullptr);
            LineTo(hdc, x + tile * 5 / 6, y + tile / 5);
            MoveToEx(hdc, x + tile / 4, y + tile * 2 / 3, nullptr);
            LineTo(hdc, x + tile / 6, y + tile * 4 / 5);
            MoveToEx(hdc, x + tile * 3 / 4, y + tile * 2 / 3, nullptr);
            LineTo(hdc, x + tile * 5 / 6, y + tile * 4 / 5);
            SelectObject(hdc, oldPen);
            DeleteObject(prong);
            break;
        }
        default:
        {
            RECT inner = {x + tile / 4, y + tile / 4, x + tile * 3 / 4, y + tile * 3 / 4};
            fill(hdc, inner, RGB(26, 118, 216));
            frame(hdc, inner, RGB(255, 255, 255), 2);
            break;
        }
        }
    }

    void drawExplosionMark(HDC hdc, int cx, int cy, int tile)
    {
        HPEN white = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
        HPEN pink = CreatePen(PS_SOLID, 2, RGB(255, 120, 220));
        HGDIOBJ oldPen = SelectObject(hdc, white);
        MoveToEx(hdc, cx - tile / 2, cy, nullptr); LineTo(hdc, cx + tile / 2, cy);
        MoveToEx(hdc, cx, cy - tile / 2, nullptr); LineTo(hdc, cx, cy + tile / 2);
        SelectObject(hdc, pink);
        MoveToEx(hdc, cx - tile / 3, cy - tile / 3, nullptr); LineTo(hdc, cx + tile / 3, cy + tile / 3);
        MoveToEx(hdc, cx + tile / 3, cy - tile / 3, nullptr); LineTo(hdc, cx - tile / 3, cy + tile / 3);
        SelectObject(hdc, oldPen);
        DeleteObject(white);
        DeleteObject(pink);
    }

    void drawExplosionBurst(HDC hdc, int cx, int cy, int tile, int ticks, int totalTicks)
    {
        const int age = totalTicks - ticks;
        const int outer = tile / 2 - age * tile / 24;
        const int inner = tile / 4 + age * tile / 20;
        RECT fire = {cx - outer, cy - outer, cx + outer, cy + outer};
        RECT core = {cx - inner, cy - inner, cx + inner, cy + inner};
        fill(hdc, fire, RGB(240, 76, 36));
        fill(hdc, core, RGB(255, 218, 82));

        HPEN smoke = CreatePen(PS_SOLID, 2, RGB(80, 80, 76));
        HGDIOBJ oldPen = SelectObject(hdc, smoke);
        MoveToEx(hdc, cx - outer, cy, nullptr);
        LineTo(hdc, cx + outer, cy);
        MoveToEx(hdc, cx, cy - outer, nullptr);
        LineTo(hdc, cx, cy + outer);
        SelectObject(hdc, oldPen);
        DeleteObject(smoke);
    }

    void drawBlastShellBullet(HDC hdc, int cx, int cy, int tile, Direction direction)
    {
        const int longRadius = tile / 5;
        const int shortRadius = tile / 9;
        RECT body;
        RECT flame;
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

        fill(hdc, body, RGB(46, 46, 52));
        frame(hdc, body, RGB(255, 210, 70), 2);
        fill(hdc, flame, RGB(255, 104, 38));
        RECT core = {cx - tile / 14, cy - tile / 14, cx + tile / 14, cy + tile / 14};
        fill(hdc, core, RGB(255, 244, 150));
    }

    void drawSpawnWarning(HDC hdc, int cx, int cy, int tile, int ticks)
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

    void drawAirStrikeWarning(HDC hdc, int cx, int cy, int tile, int ticks)
    {
        const COLORREF color = (ticks / 2) % 2 == 0 ? RGB(255, 40, 40) : RGB(255, 200, 200);
        HPEN pen = CreatePen(PS_SOLID, 3, color);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        MoveToEx(hdc, cx - tile / 2, cy - tile / 2, nullptr);
        LineTo(hdc, cx + tile / 2, cy + tile / 2);
        MoveToEx(hdc, cx + tile / 2, cy - tile / 2, nullptr);
        LineTo(hdc, cx - tile / 2, cy + tile / 2);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    void drawLaserTrace(HDC hdc, int cx, int cy, int tile, char glyph, bool fromPlayer)
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

    void drawTank(HDC hdc, int x, int y, int tile, COLORREF color, char glyph)
    {
        RECT leftTrack = {x + tile / 7, y + tile / 5, x + tile / 3, y + tile * 4 / 5};
        RECT rightTrack = {x + tile * 2 / 3, y + tile / 5, x + tile * 6 / 7, y + tile * 4 / 5};
        RECT body = {x + tile / 4, y + tile / 4, x + tile * 3 / 4, y + tile * 3 / 4};
        fill(hdc, leftTrack, darken(color));
        fill(hdc, rightTrack, darken(color));
        fill(hdc, body, color);
        frame(hdc, leftTrack, RGB(255, 255, 255), 1);
        frame(hdc, rightTrack, RGB(255, 255, 255), 1);
        frame(hdc, body, RGB(255, 255, 255), 2);
        RECT turret = {x + tile * 3 / 8, y + tile * 3 / 8, x + tile * 5 / 8, y + tile * 5 / 8};
        fill(hdc, turret, lighten(color));

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
        fill(hdc, barrel, RGB(255, 255, 255));
    }

    void drawEnemyTank(HDC hdc, int x, int y, int tile, EnemyKind kind, char glyph)
    {
        COLORREF color = RGB(210, 218, 224);
        switch (kind)
        {
        case EnemyKind::Scout:
            color = RGB(205, 218, 230);
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
        }

        drawTank(hdc, x, y, tile, color, glyph);

        if (kind == EnemyKind::Armor)
        {
            RECT plate = {x + tile / 4, y + tile / 6, x + tile * 3 / 4, y + tile / 4};
            fill(hdc, plate, RGB(255, 238, 160));
        }
        else if (kind == EnemyKind::LaserGuard)
        {
            RECT lens = {x + tile / 2 - 3, y + tile / 2 - 3, x + tile / 2 + 4, y + tile / 2 + 4};
            fill(hdc, lens, RGB(255, 255, 255));
            frame(hdc, lens, RGB(80, 220, 255), 2);
        }
        else if (kind == EnemyKind::Bomber)
        {
            RECT bomb = {x + tile / 6, y + tile / 6, x + tile / 3, y + tile / 3};
            fill(hdc, bomb, RGB(40, 40, 40));
            frame(hdc, bomb, RGB(255, 230, 80), 1);
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
            fill(hdc, crown, RGB(255, 235, 80));
            frame(hdc, crown, RGB(255, 255, 255), 1);
            if (kind == EnemyKind::FinalBoss)
            {
                RECT core = {x + tile / 3, y + tile / 3, x + tile * 2 / 3, y + tile * 2 / 3};
                frame(hdc, core, RGB(255, 255, 255), 3);
            }
        }
    }

    void drawTankEmblem(HDC hdc, int x, int y, int scale, COLORREF color)
    {
        RECT body = {x, y + 24 * scale, x + 52 * scale, y + 42 * scale};
        RECT turret = {x + 16 * scale, y + 10 * scale, x + 36 * scale, y + 28 * scale};
        RECT barrel = {x + 34 * scale, y + 17 * scale, x + 62 * scale, y + 23 * scale};
        fill(hdc, body, color);
        fill(hdc, turret, lighten(color));
        fill(hdc, barrel, RGB(38, 47, 42));
    }

    void addButton(HDC hdc, int x, int y, int width, int height, const std::wstring& label, int id)
    {
        Button button = {{x, y, x + width, y + height}, label, id};
        const bool selected = static_cast<int>(buttons_.size()) == selectedButton_;
        buttons_.push_back(button);
        fill(hdc, button.rect, selected ? RGB(76, 94, 73) : RGB(31, 38, 37));
        frame(hdc, button.rect, selected ? RGB(232, 226, 190) : RGB(79, 96, 88), 3);
        text(hdc, x + 26, y + 14, label, 22, RGB(232, 226, 190), true);
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
