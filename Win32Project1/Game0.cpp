#include <stdio.h>
#include <ctime>
#include <cstdlib>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <GL/glew.h>
#include "shaders.h"
#include "Texture.h"
#include "maths.h"

#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

//#############################################################################

const GLfloat GRAVITY = -0.001f;
const GLfloat MAX_STEP_HEIGHT = 0.55f;

enum MovementState {
    OnGround,
    Falling,
    Flying,
    Swimming,
    God,
};

//#############################################################################

enum Sides {
    N = 0, E, S, W, U, D,
    Size
};


uint32_t grid[10][10][Sides::Size];

//#############################################################################

struct Player {

    Player();

    void get_eye(float *look) const;
    void get_pos(float *look) const;

    void update_gaze(GLfloat elevationDelta, GLfloat rotationDelta);

    const float PLAYER_HEIGHT = 0.75f;
    
    // position and view within the world
    float _eyeHeight = 1.5f;
    float _pos[3] = { 0,0,0 };
    float _facing = 45 * 3.14159f / 180;    // radians left/right around player axis
    float _facingUpDown = -45 * 3.14159f / 180;    // radians up/down with player eyes as level zero

                                                   // velocities
    float _upV = 0; // negative is down
    float _rightV = 0;  // negative is left
    float _forwardV = 0; // negative is backwards
    MovementState _movementState = OnGround;
};


static Player player;

Player::Player() {
}

void Player::get_eye(float *look) const {
    look[0] = _pos[0] + cosf(_facingUpDown) * cosf(_facing);
    look[1] = _pos[1] + cosf(_facingUpDown) * sinf(_facing);
    look[2] = _pos[2] + sinf(_facingUpDown) + PLAYER_HEIGHT;
}

void Player::get_pos(float *pos) const {
    pos[0] = _pos[0];
    pos[1] = _pos[1];
    pos[2] = _pos[2] + PLAYER_HEIGHT;
}

void Player::update_gaze(GLfloat elevationDelta, GLfloat rotationDelta) {
    _facingUpDown -= elevationDelta;
    _facingUpDown = max(-3.14159f / 2, _facingUpDown);
    _facingUpDown = min(3.14159f / 2, _facingUpDown);

    _facing -= rotationDelta;
}

//#############################################################################
/*======================================================================*
* glfw interface code
*======================================================================*/

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // first check for escape
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    // check for moving keys, if in a moving-mode
    int forwardMotion = 0;
    int sidewaysMotion = 0;
    int upMotion = 0;

    bool forwardKey = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool backKey = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool leftKey = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool rightKey = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    bool jumpKey = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool diveKey = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    if (forwardKey && !backKey) {
        forwardMotion = 1;
    } else if (backKey && !forwardKey) {
        forwardMotion = -1;
    }
    if (leftKey && !rightKey) {
        sidewaysMotion = 1;
    } else if (rightKey && !leftKey) {
        sidewaysMotion = -1;
    }

    if (jumpKey && !diveKey) {
        upMotion = 1;
    } else if (diveKey && !jumpKey) {
        upMotion = -1;
    }
    //if (key == GLFW_KEY_F && action == GLFW_PRESS) {
    //    if (playerState == Flying) {
    //        playerState = Falling;
    //    } else {
    //        playerState = Flying;
    //    }
    //}

    //update_player_motion(forwardMotion, sidewaysMotion, upMotion);
}

static double xposPrev, yposPrev;   // gets initialized in init_glfw()
const GLfloat SCALED_MOUSE_MOVEMENT = 0.001f;
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    GLfloat elevationDelta = (GLfloat)(ypos - yposPrev) * SCALED_MOUSE_MOVEMENT;
    GLfloat rotationDelta = (GLfloat)(xpos - xposPrev) * SCALED_MOUSE_MOVEMENT;
    yposPrev = ypos;
    xposPrev = xpos;
    player.update_gaze(elevationDelta, rotationDelta);
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetCursorPos(window, &xposPrev, &yposPrev);
    return window;
}

static void shutdown_glfw(GLFWwindow *window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

//#############################################################################
GLuint CubeID;
GLuint SquareTexturedVA;
TextureShader *textureShader;
Texture *brick1Tex;


struct Scene {
    float _ratio;

    Scene(float ratio) : _ratio(ratio) {}

    void render(const float *pos, const float *look);
};

void Scene::render(const float *pos, const float *look) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float MVP[16], view[16], projection[16];
    lookAt(pos[0], pos[1], pos[2], look[0], look[1], look[2], 0, 0, 1, view);
    perspective(45, _ratio, 0.1f, 100.0f, projection);
    matmul(projection, view, MVP);

    // draw a floor squares
    {
        glBindVertexArray(SquareTexturedVA);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        textureShader->use();
        brick1Tex->bind();
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(textureShader->samplerID, 0);
        GLfloat model[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, -0.5,-0.5,0,1 };
        GLfloat squareMVP[16];
        matmul(MVP, model, squareMVP);
        for (int y = 0; y < 10; ++y) {
            for (int x = 0; x < 10; ++x) {
                if (grid[y][x][Sides::D] != 0) {
                    GLfloat finalMat[16];
                    GLfloat locationMat[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, (GLfloat)x,(GLfloat)y,0,1 };
                    matmul(squareMVP, locationMat, finalMat);
                    glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                }
                if (grid[y][x][Sides::S] != 0) {
                    GLfloat finalMat[16];
                    GLfloat locationMat[16] = { 1,0,0,0, 0,0,1,0, 0,0,1,0, (GLfloat)x,(GLfloat)y,0,1 };
                    matmul(squareMVP, locationMat, finalMat);
                    glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                }
                if (grid[y][x][Sides::W] != 0) {
                    GLfloat finalMat[16];
                    GLfloat locationMat[16] = { 1,0,1,0, 0,1,0,0, 0,0,1,0, (GLfloat)x,(GLfloat)y,0,1 };
                    matmul(squareMVP, locationMat, finalMat);
                    glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                }
            }
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
}


void init_opengl_objects() {
    GLuint tempBufID;

//    GLuint SquareTexturedVA;
    glGenVertexArrays(1, &SquareTexturedVA);
    glBindVertexArray(SquareTexturedVA);
    GLfloat squareData[4 * 4] = {
        // 4 sets of:  2d coords, 2d tex coords
        0,0, 0,0,  0,1, 0,1,  1,0, 1,0,  1,1, 1,1 };
    GLint squareIndexes[2 * 3] = { 0,1,2, 1,2,3 };

    glGenBuffers(1, &tempBufID);
    glBindBuffer(GL_ARRAY_BUFFER, tempBufID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareData), squareData, GL_STATIC_DRAW);

    glGenBuffers(1, &tempBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempBufID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndexes), squareIndexes, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,  // attribute 0.
        2,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        4 * sizeof(GLfloat),  // stride
        (void*)0 // array buffer offset
    );

    glVertexAttribPointer(
        1,
        2,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        4 * sizeof(GLfloat),  // stride
        (void*)(2 * sizeof(GLfloat)) // array buffer offset
    );


//    GLuint CubeID;
    glGenVertexArrays(1, &CubeID);
    glBindVertexArray(CubeID);
    GLfloat cubeData[12 * 5] = {
        0,0,0, 0,0,  0,1,0, 1,0,  0,1,1, 1,1,  0,0,1, 0,1,  1,1,0, 0,0,  1,1,1, 0,1,  1,0,0, 1,0,  1,0,1, 1,1,
        1,0,1, 0,0,  0,0,1, 1,0,  1,0,0, 0,1,  0,0,0, 1,1 };
    GLint cubeIndexes[8 * 4] = { 0,1,2,3, 1,4,5,2, 4,6,7,5, 6,0,3,7, 2,5,8,9, 1,4,10,11 };

    glGenBuffers(1, &tempBufID);
    glBindBuffer(GL_ARRAY_BUFFER, tempBufID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeData), cubeData, GL_STATIC_DRAW);

    glGenBuffers(1, &tempBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempBufID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndexes), cubeIndexes, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        5 * sizeof(GLfloat),  // stride
        (void*)0 // array buffer offset
    );

    glVertexAttribPointer(
        1,
        2,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        5 * sizeof(GLfloat),  // stride
        (void*)(3 * sizeof(GLfloat)) // array buffer offset
    );


    // clear the bound buffer array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    
    textureShader = new TextureShader("..\\shaders\\SimpleTextureShader.vert", "..\\shaders\\SimpleTextureShader.frag");
    brick1Tex = new Texture(std::wstring(L"..\\textures\\brick_0.jpg"));

    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);
}

//#############################################################################

void init_game_objects() {
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            for (int side = 0; side < 6; ++side) {
                grid[y][x][side] = 0;
            }
        }
    }
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 4; ++x) {
            grid[y][x][Sides::D] = 1;
        }
    }
    grid[3][1][Sides::D] = 1;
    grid[4][1][Sides::D] = 1;
    grid[4][0][Sides::D] = 1;
    grid[4][0][Sides::D] = 1;

    grid[0][0][Sides::S] = 1;
    grid[0][1][Sides::S] = 1;
    grid[0][2][Sides::S] = 1;
    grid[0][3][Sides::S] = 1;

    grid[0][0][Sides::W] = 1;
    grid[1][0][Sides::W] = 1;

}

//#############################################################################

int main(void) {
    GLFWwindow *window = init_glfw();
    init_opengl_objects();
    init_game_objects();

    float ratio;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;
    glViewport(0, 0, width, height);
    Scene scene(ratio);

    clock_t startTime = clock(); //Start timer
    while (!glfwWindowShouldClose(window)) {

        float pos[3];
        float look[3];
        player.get_pos(pos);
        player.get_eye(look);  // get eye so we know which direction forward is
        scene.render(pos, look);

        glfwSwapBuffers(window);
        glfwPollEvents();
        //update_object_locations();
    }
    shutdown_glfw(window);
}