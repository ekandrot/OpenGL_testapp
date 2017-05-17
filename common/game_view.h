#pragma once

#include <memory>

struct GameView {
    GameView();
    ~GameView();

    void pollEvents();
    void swapBuffers();
    bool shouldQuit();
private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};
