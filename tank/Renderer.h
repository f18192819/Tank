#pragma once

#include <string>
#include <vector>

class Game;

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void draw(const Game& game);
    void showTitle();

private:
    void moveCursorHome() const;
    void hideCursor() const;
    void showCursor() const;
};
