#include "game_view.h"

#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>


#include <stdio.h>
#include <cstdlib>

//#############################################################################

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // first check for escape
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == 0 && action == GLFW_PRESS) {
    }
}


static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
}


static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

//#############################################################################

static GLFWwindow* init_glfw() {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // We want OpenGL 4.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 16);

    GLFWwindow* window = glfwCreateWindow(960, 960, "Masterful", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    //glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(EXIT_FAILURE);
    }

    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//    glfwGetCursorPos(window, &xposPrev, &yposPrev);
    return window;
}

//#############################################################################

static void shutdown_glfw(GLFWwindow *window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwDestroyWindow(window);
    glfwTerminate();
}

//#############################################################################

struct GameView::impl {
    GLFWwindow *window;
};

GameView::GameView() : pimpl{new impl{} } {
    GLFWwindow *window = init_glfw();
    pimpl->window = window;
}

GameView::~GameView() {
    shutdown_glfw(pimpl->window);
}

        // on the latest Ubuntu with the latest GLFW, I'm seeing key sequences:
        //  press, repeat, release, press, repeat, release - when there was only one press/release
        //  there was a bug that was fixed that was supposed to fix this, but I'm still seeing it.
        //  it is from event polling not being called frequently enough.
        //  I found by adding these 4 calls to poll events here, it minimizes this bug.
        //  revisit when https://github.com/glfw/glfw/issues/747 is truly fixed.  05/12/2017
void GameView::pollEvents() {
    glfwPollEvents();
}

void GameView::swapBuffers() {
    glfwSwapBuffers(pimpl->window);
}

bool GameView::shouldQuit() {
    return glfwWindowShouldClose(pimpl->window);
}

//#############################################################################
//#############################################################################

