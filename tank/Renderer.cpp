#include "Renderer.h"

#include <iostream>
#include <windows.h>

#include "Game.h"

Renderer::Renderer()
{
    hideCursor();
}

Renderer::~Renderer()
{
    showCursor();
}

void Renderer::draw(const Game& game)
{
    std::vector<std::string> buffer(GameMap::Height, std::string(GameMap::Width, ' '));
    game.draw(buffer);

    std::string output;
    output.reserve((GameMap::Width + 3) * (GameMap::Height + 6));
    for (std::size_t y = 0; y < buffer.size(); ++y)
    {
        output += buffer[y];
        output += "\n";
    }

    output += "Lives: " + std::to_string(game.playerLives());
    output += "  Score: " + std::to_string(game.score());
    output += "  Enemies: " + std::to_string(game.enemyCount());
    output += "  Level: " + std::to_string(game.wave());
    if (game.bossMaxLives() > 0)
    {
        output += "  Boss: " + std::to_string(game.bossLives()) + "/" + std::to_string(game.bossMaxLives());
    }
    output += "\nItems  Shield:" + std::to_string(game.shieldCharges());
    output += "  F-Bomb:" + std::to_string(game.bombShells());
    output += "  E-Laser:" + std::to_string(game.lasers());
    output += "  Q-Shovel:" + std::to_string(game.shovels());
    output += "  G-Decoy:" + std::to_string(game.decoys());
    output += "\nWASD move  J fire  T trench  R restart  Esc quit\n";

    if (game.state() == GameState::Victory)
    {
        output += "Victory! Press R to play again, or Esc to quit.     \n";
    }
    else if (game.state() == GameState::Defeat)
    {
        output += "Defeat! Press R to try again, or Esc to quit.       \n";
    }
    else
    {
        output += "% crate # wall T trench ~ swamp N normal E elite    \n";
    }

    moveCursorHome();
    std::cout << output;
}

void Renderer::showTitle()
{
    system("cls");
    std::cout << "Tank Battle\n";
    std::cout << "WASD move, J fire, F bomb, E laser, Q shovel, G decoy, Esc quit\n\n";
}

void Renderer::moveCursorHome() const
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD position = {0, 0};
    SetConsoleCursorPosition(handle, position);
}

void Renderer::hideCursor() const
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(handle, &info);
    info.bVisible = FALSE;
    SetConsoleCursorInfo(handle, &info);
}

void Renderer::showCursor() const
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(handle, &info);
    info.bVisible = TRUE;
    SetConsoleCursorInfo(handle, &info);
}
