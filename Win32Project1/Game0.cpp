#include <stdio.h>
#include <ctime>
#include <cstdlib>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
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

uint32_t grid[10][10];

static inline bool is_blocking(uint32_t gridElement) {
    if (gridElement < 1024) return true;
    return false;
}

//#############################################################################

struct MovementInputState {
    bool _forwardKey;
    bool _backwardKey;
    bool _leftKey;
    bool _rightKey;
    bool _jumpKey;
    bool _diveKey;
    bool _runningKey;
};
MovementInputState gMovementInputState;

//#############################################################################

struct Player {

    Player();

    void update_eye_pos();
    void get_eye_look(float *look) const;
    void get_eye_pos(float *look) const;

    void update_gaze(GLfloat elevationDelta, GLfloat rotationDelta);
    void update_movement(const MovementInputState &movementInputState);
    void update_velocities(int forwardMotion, int sidewaysMotion, int upMotion);
    void update_pos(const GLfloat* look);

    void update_pos_onground(const GLfloat* look);
    void update_pos_falling(const GLfloat* look);
    void update_pos_flying(const GLfloat* look);
    
    // position and view within the world
    bool _crouching = false;
    bool _running = false;
    float _eyeHeight = 1.5f;
    float _pos[3] = { 1,1,0 };
    float _facing = 45 * 3.14159f / 180;    // radians left/right around player axis
    float _facingUpDown = -45 * 3.14159f / 180;    // radians up/down with player eyes as level zero

                                                   // velocities
    float _upV = 0; // negative is down
    float _rightV = 0;  // negative is left
    float _forwardV = 0; // negative is backwards
    MovementState _movementState = OnGround;
};


static Player player;

void Player::update_pos_onground(const GLfloat* look) {
    // estimate a new position for the player
    // newpos is based on:
    //   if player has a velocity, reduce it by the blocktype
    // does newpos put player into a new tile?  if so:
    //   would he now be falling?
    //   is the height difference acceptable?
    // if still onground, update velocity by keys pressed
    // accept newpos if it is valid

    GLfloat Z[3] = { look[0] - _pos[0], look[1] - _pos[1], 0 }; //, look[2] - pos[2]};
    normalize(Z);
    GLfloat newPos[3];
    newPos[0] = _pos[0] + Z[0] * _forwardV - Z[1] * _rightV;
    newPos[1] = _pos[1] + Z[1] * _forwardV + Z[0] * _rightV;
    newPos[2] = _pos[2] + _upV;

    bool validMove = false;
    // check falling off grid

    // the indexes of where we are on the grid
    int ix = (int)_pos[0];
    int iy = (int)_pos[1];

    // check for walls where we are, and prevent moving through them
    if (is_blocking(grid[iy][ix-1])) {
        newPos[0] = max(newPos[0], ix + 0.15);
    }
    if (is_blocking(grid[iy][ix+1])) {
        newPos[0] = min(newPos[0], ix + 0.85);
    }
    if (is_blocking(grid[iy-1][ix])) {
        newPos[1] = max(newPos[1], iy + 0.15);
    }
    if (is_blocking(grid[iy+1][ix])) {
        newPos[1] = min(newPos[1], iy + 0.85);
    }

    // make sure we stay within the grid, overall
    newPos[0] = max(0.15, min(9.85, newPos[0]));
    newPos[1] = max(0.15, min(9.85, newPos[1]));

  /*  if ((newPos[0] <= 0) || (newPos[0] >= 10) || (newPos[1] <= 0) || (newPos[1] >= 10)) {
        //playerState = Falling;
        //validMove = true;
    } else */{
        //printf("%d, %d\n", (int)(newPos[0]*5 + 5), (int)(newPos[1]*5 + 5));
        float groundHeight = 0;
        float myHeight = newPos[2];
        if (groundHeight - myHeight <= MAX_STEP_HEIGHT) {
            validMove = true;
            if (newPos[2] <= groundHeight) {
                newPos[2] = groundHeight;
            } else if (newPos[2] > groundHeight) {
                _movementState = Falling;
            }
        }
    }

    if (validMove) {
        _pos[0] = newPos[0];
        _pos[1] = newPos[1];
        _pos[2] = newPos[2];
    }
    //facing += rotationV;        // need to include a timestamp to handle fps correctly
}

void Player::update_pos_falling(const GLfloat* look) {
    _upV += GRAVITY;

    GLfloat Z[3] = { look[0] - _pos[0], look[1] - _pos[1], 0 }; //, look[2] - pos[2]};
    normalize(Z);
    GLfloat newPos[3];
    newPos[0] = _pos[0] + Z[0] * _forwardV - Z[1] * _rightV;
    newPos[1] = _pos[1] + Z[1] * _forwardV + Z[0] * _rightV;
    newPos[2] = _pos[2] + _upV;

    // the indexes of where we are on the grid
    int ix = (int)_pos[0];
    int iy = (int)_pos[1];

    // check for walls where we are, and prevent moving through them
    if (is_blocking(grid[iy][ix - 1])) {
        newPos[0] = max(newPos[0], ix + 0.15);
    }
    if (is_blocking(grid[iy][ix + 1])) {
        newPos[0] = min(newPos[0], ix + 0.85);
    }
    if (is_blocking(grid[iy - 1][ix])) {
        newPos[1] = max(newPos[1], iy + 0.15);
    }
    if (is_blocking(grid[iy + 1][ix])) {
        newPos[1] = min(newPos[1], iy + 0.85);
    }

    float groundHeight = 0;
    if (newPos[2] < groundHeight) {
        newPos[2] = groundHeight;
        _upV = 0;
        _movementState = OnGround;
    }

    bool validMove = true;
    if (validMove) {
        _pos[0] = newPos[0];
        _pos[1] = newPos[1];
        _pos[2] = newPos[2];
    }
    //facing += rotationV;        // need to include a timestamp to handle fps correctly
}

void Player::update_pos_flying(const GLfloat* look) {
    GLfloat Z[3] = { look[0] - _pos[0], look[1] - _pos[1], 0 }; //, look[2] - pos[2]};
    normalize(Z);
    GLfloat newPos[3];
    newPos[0] = _pos[0] + Z[0] * _forwardV - Z[1] * _rightV;
    newPos[1] = _pos[1] + Z[1] * _forwardV + Z[0] * _rightV;
    newPos[2] = _pos[2] + _upV;

    bool validMove = true;
    if (validMove) {
        _pos[0] = newPos[0];
        _pos[1] = newPos[1];
        _pos[2] = newPos[2];
    }
    //facing += rotationV;        // need to include a timestamp to handle fps correctly
}

/*
*   First calculate where we are, then the direction we are looking from that position
*/
void Player::update_pos(const GLfloat* look) {
    switch (_movementState) {
        case OnGround:
            update_pos_onground(look);
            break;
        case Falling:
            update_pos_falling(look);
            break;
        case Flying:
            update_pos_flying(look);
            break;
        default:
            break;
    }
}



Player::Player() {
}

void Player::update_eye_pos() {
    if (_crouching) {
        if (_eyeHeight > 0.25) {
            _eyeHeight -= 0.01;
        } else {
            _eyeHeight = 0.25;
        }
    } else {
        if (_eyeHeight < 0.75) {
            _eyeHeight += 0.01;
        } else {
            _eyeHeight = 0.75;
        }
    }
}

void Player::get_eye_look(float *look) const {
    look[0] = _pos[0] + cosf(_facingUpDown) * cosf(_facing);
    look[1] = _pos[1] + cosf(_facingUpDown) * sinf(_facing);
    look[2] = _pos[2] + sinf(_facingUpDown) + _eyeHeight;
}

void Player::get_eye_pos(float *pos) const {
    pos[0] = _pos[0];
    pos[1] = _pos[1];
    pos[2] = _pos[2] +_eyeHeight;
}

void Player::update_velocities(int forwardMotion, int sidewaysMotion, int upMotion) {
    const double maxSpeed = (_crouching ? 0.008 : (_running ? 0.035 : 0.017));
    const double speeds[4] = { 0, 1, 1 / sqrt(2), 1 / sqrt(3) };

    switch (_movementState) {
        case OnGround:
            if (upMotion > 0) {
                _movementState = Falling;
                _upV += 0.05f;
            }
        case Falling: {
            int speed = abs(forwardMotion) + abs(sidewaysMotion);
            double d = maxSpeed * speeds[speed];
            _forwardV = forwardMotion * d;
            _rightV = sidewaysMotion * d;
            break;
        }
        case Flying: {
            int speed = abs(forwardMotion) + abs(sidewaysMotion) + abs(upMotion);
            double d = maxSpeed * speeds[speed];
            _forwardV = forwardMotion * d;
            _rightV = sidewaysMotion * d;
            _upV = upMotion * d;
            break;
        }
        default:
            break;
    }
}

void Player::update_movement(const MovementInputState &m) {
    int forwardMotion=0, sidewaysMotion=0, upMotion=0;
    if (m._forwardKey && !m._backwardKey) {
        forwardMotion = 1;
    } else if (m._backwardKey && !m._forwardKey) {
        forwardMotion = -1;
    }
    if (m._leftKey && !m._rightKey) {
        sidewaysMotion = 1;
    } else if (m._rightKey && !m._leftKey) {
        sidewaysMotion = -1;
    }

    _crouching = false;
    if (m._jumpKey && !m._diveKey) {
        upMotion = 1;
    } else if (m._diveKey && !m._jumpKey) {
        upMotion = -1;
        if (_movementState == OnGround) {
            _crouching = true;
        }
    }

    _running = false;
    if (m._runningKey) {
        _running = true;
    }

    update_velocities(forwardMotion, sidewaysMotion, upMotion);
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

    // action can be GLFW_PRESS or GLFW_REPEAT to count as down
    // action will be GLFW_RELEASE when the key is released
    switch (key) {
        case GLFW_KEY_W:
            gMovementInputState._forwardKey = (action != GLFW_RELEASE);
            break;
        case GLFW_KEY_S:
            gMovementInputState._backwardKey = (action != GLFW_RELEASE);
            break;
        case GLFW_KEY_A:
            gMovementInputState._leftKey = (action != GLFW_RELEASE);
            break;
        case GLFW_KEY_D:
            gMovementInputState._rightKey = (action != GLFW_RELEASE);
            break;
        case GLFW_KEY_SPACE:
            gMovementInputState._jumpKey = (action != GLFW_RELEASE);
            break;
        case GLFW_KEY_C:
            gMovementInputState._diveKey = (action != GLFW_RELEASE);
            break;
        case GLFW_KEY_LEFT_SHIFT:
            gMovementInputState._runningKey = (action != GLFW_RELEASE);
            break;
        case GLFW_KEY_F:
            if (action == GLFW_PRESS) {
                if (player._movementState == Flying) {
                    player._movementState = Falling;
                } else {
                    player._movementState = Flying;
                }
            }
            break;
        default:
            break;
    }
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

//  *  @param[in] action   One of `GLFW_PRESS` or `GLFW_RELEASE`.
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == 0 && action == GLFW_PRESS) {
    }
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
std::map<uint32_t, Texture*> textureDict;
Texture *monsterTex;


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

    // draw monsters!
    {
        glBindVertexArray(CubeID);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        textureShader->use();
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(textureShader->samplerID, 0);
        GLfloat model[16] = { 0.5,0,0,0, 0,0.5,0,0, 0,0,0.5,0, 7.25,3.25,0.25,1 };
        GLfloat squareMVP[16];
        matmul(MVP, model, squareMVP);
        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, squareMVP);
        monsterTex->bind();
        glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, nullptr);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }


    // draw a floor squares
    {
        glBindVertexArray(SquareTexturedVA);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        textureShader->use();
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(textureShader->samplerID, 0);
        GLfloat finalMat[16];
        for (int y = 0; y < 10; ++y) {
            for (int x = 0; x < 10; ++x) {
                textureDict[grid[y][x]]->bind();
                if (!is_blocking(grid[y][x])) {
                    // draw floor for non-blocking spaces
                    GLfloat locationMat[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, (GLfloat)x,(GLfloat)y,0,1 };
                    matmul(MVP, locationMat, finalMat);
                    glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                }

                if (is_blocking(grid[y][x])) {
                    if (y>0 && !is_blocking(grid[y-1][x])) {
                        // only draw South face if there is an non-blocking element to the south
                        GLfloat locationMat[16] = { -1,0,0,0, 0,0,1,0, 0,0,0,0, (GLfloat)x + 1,(GLfloat)y,0,1 };
                        matmul(MVP, locationMat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                    }
                    if (y < 9 && !is_blocking(grid[y+1][x])) {
                        // only draw South face if there is an non-blocking element to the north
                        GLfloat locationMat[16] = { 1,0,0,0, 0,0,1,0, 0,0,0,0, (GLfloat)x,(GLfloat)y + 1,0,1 };
                        matmul(MVP, locationMat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                    }

                    if (x > 0 && !is_blocking(grid[y][x-1])) {
                        GLfloat locationMat[16] = { 0,1,0,0, 0,0,1,0, 0,0,0,0, (GLfloat)x,(GLfloat)y,0,1 };
                        matmul(MVP, locationMat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                    }

                    if (x < 9 && !is_blocking(grid[y][x+1])) {
                        GLfloat locationMat[16] = { 0,-1,0,0, 0,0,1,0, 0,0,0,0, (GLfloat)x + 1,(GLfloat)y + 1,0,1 };
                        matmul(MVP, locationMat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                    }

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

    textureDict[1] = new Texture(std::wstring(L"..\\textures\\wall_0.jpg"));
    textureDict[1024] = new Texture(std::wstring(L"..\\textures\\dirt_floor_0.png"));
    monsterTex = new Texture(std::wstring(L"..\\textures\\dummy_0.jpg"));

    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);
}

//#############################################################################

void init_game_objects() {
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            for (int side = 0; side < 6; ++side) {
                grid[y][x] = 1024;
            }
        }
    }
    for (int i = 0; i < 10; ++i) {
        grid[0][i] = 1;
        grid[9][i] = 1;
        grid[i][0] = 1;
        grid[i][9] = 1;
    }

    // build a small room for testing
    grid[4][1] = 1;
    grid[4][3] = 1;
    grid[4][4] = 1;
    grid[4][5] = 1;
    grid[3][5] = 1;
    grid[2][5] = 1;
    grid[1][5] = 1;

    grid[5][3] = 1;
    grid[6][2] = 1;
    grid[6][3] = 1;
}

//#############################################################################

void update_object_locations(float *look) {
    player.update_movement(gMovementInputState);
    player.update_pos(look);
}

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
        player.update_eye_pos();
        player.get_eye_pos(pos);
        player.get_eye_look(look);  // get eye so we know which direction forward is
        scene.render(pos, look);

        glfwSwapBuffers(window);
        glfwPollEvents();
        update_object_locations(look);
    }
    shutdown_glfw(window);
}