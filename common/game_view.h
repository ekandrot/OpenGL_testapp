#pragma once

//GLFWwindow* init_glfw();
//
//void shutdown_glfw(GLFWwindow *window);

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
