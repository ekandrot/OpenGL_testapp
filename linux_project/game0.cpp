#include <stdio.h>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include "shaders.h"
#include "texture.h"
#include "maths.h"

//#define GLFW_INCLUDE_GLU
//#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include "chunk.h"
#include "constants.h"
#include "player.h"

// queues of events, captured in callbacks, used during update ticks of the engine
std::vector<std::pair<float, float> >  captured_mouse_movements;
std::vector<std::pair<int, int> >  captured_keyboard_inputs;    // key, status



// GLfloat *roach_data;
// GLint *roach_indexes;
// GLsizeiptr sizeof_roach_data;
// GLsizeiptr sizeof_roach_indexes;
// int count_roach_indexes;

const float CURSOR_SIZE = 0.05;

//#############################################################################

enum GameMode {
    Playing,
    Inventory,
};

//#############################################################################

static inline void my_assign(GLfloat *a, std::initializer_list<float> c) {
    for (size_t i=0; i<c.size(); ++i) {
        a[i] = *(c.begin()+i);
    }
}


//#############################################################################

struct Game {
    Game() : _mode(Playing), _cursor_x(0), _cursor_y(0) {}

    GameMode _mode;
    float _cursor_x;
    float _cursor_y;
};
Game gGame;

//#############################################################################

struct moving_object {
    moving_object() : _activated(false) {}

    void activate(float x, float y, float z, float vx, float vy, float vz) {
        if (!_activated) {
            float d = sqrt(vx*vx + vy*vy + vz*vz);
            vx /= d;
            vy /= d;
            vz /= d;

            _activated = true;
            _ttl = 40;
            _posx = x;
            _posy = y;
            _posz = z;
            _vx = vx * 0.15f;
            _vy = vy * 0.15f;
            _vz = vz * 0.15f;

            _updates = 0;
        }
    }

    void update(double tick) {

        if (_activated) {
            _updates++;
            _ttl -= tick;
            if (_ttl <= 0) {
                _activated = false;
                /* debugging timing*/
                //printf("was updated %d times\n", _updates);
            } else {
                _posx += _vx*tick;
                _posy += _vy*tick;
                _posz += _vz*tick;
            }
        } 
    }

    int _updates;
    bool _activated;
    double _ttl; 
    float _posx, _posy, _posz;
    float _vx, _vy, _vz;
};

moving_object missile;

//#############################################################################

struct block_object {
    block_object() : x_(3), y_(3), z_(3), falling_(true) {}

    void update(double tick, const Chunk &chunk) {
        if (falling_) {
            z_ += vz_ * tick;
            vz_ += GRAVITY * tick;
            if (0 < chunk.grid[int(z_)][int(y_)][int(x_)]) {
                z_ = int(z_) + 1;
                vz_ = 0;
                falling_ = false;
            }
        } 
    }

    float x_, y_, z_;
    float vx_, vy_, vz_;
    bool falling_;
};

block_object falling_cube;



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

    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        if (gGame._mode == Inventory) {
            gGame._mode = Playing;
        } else {
            gGame._mode = Inventory;
        }
    }

    if (gGame._mode == Playing) {
        captured_keyboard_inputs.push_back(std::make_pair(key, action));
    }
}

static void process_keys(const std::vector<std::pair<int, int> > &keyboard_inputs, MovementInputState &player_movement_state) {
    for (auto key_press : keyboard_inputs) {
        int key = key_press.first;
        int action = key_press.second;

        // action can be GLFW_PRESS or GLFW_REPEAT to count as down
        // action will be GLFW_RELEASE when the key is released
        bool keyIsDown = (action != GLFW_RELEASE);
        switch (key) {
            case GLFW_KEY_W:
                player_movement_state._forwardKey = keyIsDown;
                break;
            case GLFW_KEY_S:
                player_movement_state._backwardKey = keyIsDown;
                break;
            case GLFW_KEY_A:
                player_movement_state._leftKey = keyIsDown;
                break;
            case GLFW_KEY_D:
                player_movement_state._rightKey = keyIsDown;
                break;
            case GLFW_KEY_SPACE:
                player_movement_state._jumpKey = keyIsDown;
                break;
            case GLFW_KEY_C:
                player_movement_state._diveKey = keyIsDown;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                player_movement_state._runningKey = keyIsDown;
                break;
            case GLFW_KEY_F:
                // if (action == GLFW_PRESS) {
                //     if (player._movementState == Flying) {
                //         player._movementState = Falling;
                //     } else {
                //         player._movementState = Flying;
                //     }
                // }
                break;

            default:
                break;
        }
    }
}


static double xposPrev, yposPrev;   // gets initialized in init_glfw()
const GLfloat SCALED_MOUSE_MOVEMENT = 0.001f;
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    if (gGame._mode == Inventory) {
        gGame._cursor_x += (GLfloat)(xpos - xposPrev) * SCALED_MOUSE_MOVEMENT;
        gGame._cursor_y -= (GLfloat)(ypos - yposPrev) * SCALED_MOUSE_MOVEMENT;
        if (gGame._cursor_x < -0.75) gGame._cursor_x = -0.75;
        else if (gGame._cursor_x > 0.75-CURSOR_SIZE) gGame._cursor_x = 0.75-CURSOR_SIZE;
        if (gGame._cursor_y < -0.75) gGame._cursor_y = -0.75;
        else if (gGame._cursor_y > 0.75-CURSOR_SIZE) gGame._cursor_y = 0.75-CURSOR_SIZE;
    } else {
        GLfloat elevationDelta = (GLfloat)(ypos - yposPrev) * SCALED_MOUSE_MOVEMENT;
        GLfloat rotationDelta = (GLfloat)(xpos - xposPrev) * SCALED_MOUSE_MOVEMENT;
        captured_mouse_movements.push_back(std::make_pair(elevationDelta, rotationDelta));
    }
    yposPrev = ypos;
    xposPrev = xpos;
}

//  *  @param[in] action   One of `GLFW_PRESS` or `GLFW_RELEASE`.
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == 0 && action == GLFW_PRESS) {
            if (action == GLFW_PRESS) {
                // float pos[3], look[3];
                // player.get_eye_pos(pos);
                // player.get_eye_look(look);
                // missile.activate(pos[0], pos[1], pos[2], look[0]-pos[0], look[1]-pos[1], look[2]-pos[2]);
            }
    }
}

static void error_callback(int error, const char* description) {
    fprintf(stderr, "*** GLFW Error: %s\n", description);
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

GLuint cube_id;
GLuint cube_outline_id;
GLuint SquareTexturedVA;
GLuint roach_id;
GLuint inventory_cells_va;
TextureShader *textureShader;
FixedColorShader *colorShader;
std::map<uint32_t, Texture*> textureDict;
Texture *monsterTex;
Texture *crosshairsTex;

struct Scene {
    float _ratio;

    Scene(float ratio) : _ratio(ratio) {}

    void render(const Chunk &chunk, const float *pos, const float *look);
};

void Scene::render(const Chunk &chunk, const float *pos, const float *look) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float MVP[16], view[16], projection[16];
    lookAt(pos[0], pos[1], pos[2], look[0], look[1], look[2], 0, 0, 1, view);
    perspective(70, _ratio, 0.1f, 100.0f, projection);
    matmul(projection, view, MVP);

    // draw monsters!
    #if 0
    {
        glBindVertexArray(roach_id);
        colorShader->use();
        //glActiveTexture(GL_TEXTURE0);
        glUniform3f(colorShader->colorID, 166/255.0, 123/255.0, 91/255.0);
        float tick = 0; // timing stuff shouldn't be in the render loop
        float monster_x = 7.25 + sin(tick/100);
        float monster_y = 3.25 + cos(tick/100);
        GLfloat model[16] = { 0.5,0,0,0, 0,0.5,0,0, 0,0,0.5,0, (float)monster_x,monster_y,0.25,1 };
        GLfloat squareMVP[16];
        matmul(MVP, model, squareMVP);
        glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, squareMVP);
        glDrawElements(GL_QUADS, count_roach_indexes, GL_UNSIGNED_INT, nullptr);
    }
    #endif

    if (1)
    {
        for (int z = 0; z < 16; ++z) {
            for (int y = 0; y < 16; ++y) {
                for (int x = 0; x < 16; ++x) {
                    if (chunk.grid[z][y][x] > 0) {
                        glBindVertexArray(cube_id);
                        colorShader->use();
                        glUniform3f(colorShader->colorID, 0/255.0, 128/255.0, 0/255.0);
                        GLfloat model[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, (float)x,(float)y,(float)z,1 };
                        GLfloat squareMVP[16];
                        matmul(MVP, model, squareMVP);
                        glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, squareMVP);
                        glDrawElements(GL_TRIANGLE_STRIP, 19, GL_UNSIGNED_INT, nullptr);

                        glBindVertexArray(cube_outline_id);
                        glUniform3f(colorShader->colorID, 0, 0, 0);
                        glLineWidth(1);
                        glDrawElements(GL_LINE_STRIP, 16, GL_UNSIGNED_INT, nullptr);
                    }
                }
            }
        }
    }


#if 0
    // draw a floor squares
    if (0)
    {
        glBindVertexArray(SquareTexturedVA);
        colorShader->use();
        //glActiveTexture(GL_TEXTURE0);
        glUniform3f(colorShader->colorID, 0/255.0, 255/255.0, 0/255.0);

        // textureShader->use();
        // glActiveTexture(GL_TEXTURE0);
        // glUniform1i(textureShader->samplerID, 0);
        GLfloat finalMat[16];
        GLfloat location_mat[16];
        for (int y = 0; y < 10; ++y) {
            for (int x = 0; x < 10; ++x) {
                textureDict[chunk.grid[0][y][x]]->bind();
                if (!is_blocking(chunk.grid[0][y][x])) {
                    // draw floor for non-blocking spaces
                    my_assign(location_mat, { 1,0,0,0, 0,1,0,0, 0,0,1,0, (GLfloat)x,(GLfloat)y,0,1 });
                    matmul(MVP, location_mat, finalMat);
                    glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                } else {
                    if (y>0 && !is_blocking(grid[y-1][x])) {
                        // only draw South face if there is an non-blocking element to the south
                        my_assign(location_mat, { -1,0,0,0, 0,0,1,0, 0,0,0,0, (GLfloat)x + 1,(GLfloat)y,0,1 });
                        matmul(MVP, location_mat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle

                        location_mat[14]++;
                        matmul(MVP, location_mat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                    }
                    if (y < 9 && !is_blocking(grid[y+1][x])) {
                        // only draw South face if there is an non-blocking element to the north
                        my_assign(location_mat, { 1,0,0,0, 0,0,1,0, 0,0,0,0, (GLfloat)x,(GLfloat)y + 1,0,1 });
                        matmul(MVP, location_mat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle

                        location_mat[14]++;
                        matmul(MVP, location_mat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                    }

                    if (x > 0 && !is_blocking(grid[y][x-1])) {
                        my_assign(location_mat, { 0,1,0,0, 0,0,1,0, 0,0,0,0, (GLfloat)x,(GLfloat)y,0,1 });
                        matmul(MVP, location_mat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle

                        location_mat[14]++;
                        matmul(MVP, location_mat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                    }

                    if (x < 9 && !is_blocking(grid[y][x+1])) {
                        my_assign(location_mat, { 0,-1,0,0, 0,0,1,0, 0,0,0,0, (GLfloat)x + 1,(GLfloat)y + 1,0,1 });
                        matmul(MVP, location_mat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle

                        location_mat[14]++;
                        matmul(MVP, location_mat, finalMat);
                        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, finalMat);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
                    }
                }
            }
        }
    }
#endif

    // if (missile._activated) {
    //     glBindVertexArray(cube_id);
    //     colorShader->use();
    //     //glActiveTexture(GL_TEXTURE0);
    //     glUniform3f(colorShader->colorID, 166/255.0, 23/255.0, 91/255.0);
    //     GLfloat model[16] = { 0.1,0,0,0, 0,0.1,0,0, 0,0,0.1,0, missile._posx,missile._posy,missile._posz,1 };
    //     GLfloat squareMVP[16];
    //     matmul(MVP, model, squareMVP);
    //     glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, squareMVP);
    //     glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, nullptr);
    // }


    {
        glBindVertexArray(cube_id);
        colorShader->use();
        //glActiveTexture(GL_TEXTURE0);
        glUniform3f(colorShader->colorID, 166/255.0, 166/255.0, 166/255.0);
        GLfloat model[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, falling_cube.x_,falling_cube.y_,falling_cube.z_,1 };
        GLfloat squareMVP[16];
        matmul(MVP, model, squareMVP);
        glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, squareMVP);
        glDrawElements(GL_TRIANGLE_STRIP, 19, GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(cube_outline_id);
        glUniform3f(colorShader->colorID, 0, 0, 0);
        glLineWidth(5);
        glDrawElements(GL_LINE_STRIP, 16, GL_UNSIGNED_INT, nullptr);
    }


    // should we render player's inventory over the game screen?
    if (gGame._mode == Inventory) {
        glBindVertexArray(SquareTexturedVA);
        colorShader->use();
        glUniform3f(colorShader->colorID, 166/255.0, 166/255.0, 166/255.0);
        GLfloat model[16] = { 1.5,0,0,0, 0,1.5,0,0, 0,0,0,0, -0.75,-0.75,-0.5,1 };
        glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, model);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(inventory_cells_va);
        colorShader->use();
        glUniform3f(colorShader->colorID, 0, 0, 0);
        GLfloat model_cells[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,-0.51,1 };
        glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, model_cells);
        glDrawElements(GL_TRIANGLE_STRIP, 6*5*10, GL_UNSIGNED_INT, nullptr);

        // draw a cursor in inventory mode
        glBindVertexArray(SquareTexturedVA);
        colorShader->use();
        glUniform3f(colorShader->colorID, 255/255.0, 166/255.0, 166/255.0);
        GLfloat model_cursor[16] = { CURSOR_SIZE,0,0,0, 0,CURSOR_SIZE,0,0, 0,0,0,0, gGame._cursor_x,gGame._cursor_y,-0.52,1 };
        glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, model_cursor);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    } else {
        // draw crosshairs in play mode?
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(SquareTexturedVA);
        textureShader->use();
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(textureShader->samplerID, 0);
        crosshairsTex->bind();
        GLfloat model[16] = { 0.1,0,0,0, 0,0.1,0,0, 0,0,0,0, -0.05,-0.05,0,1};
        glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, model);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // 3 vertex per triangle
        glDisable(GL_BLEND);
    }

    // draw the "always on" action bar
    glBindVertexArray(SquareTexturedVA);
    colorShader->use();
    glUniform3f(colorShader->colorID, 166/255.0, 166/255.0, 166/255.0);
    GLfloat model[16] = { 1.5,0,0,0, 0,0.15,0,0, 0,0,0,0, -0.75,-0.95,-0.5,1 };
    glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, model);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(inventory_cells_va);
    colorShader->use();
    glUniform3f(colorShader->colorID, 0, 0, 0);
    GLfloat model_cells[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,-0.95+0.005,-0.51,1 };
    glUniformMatrix4fv(colorShader->matrixID, 1, GL_FALSE, model_cells);
    glDrawElements(GL_TRIANGLE_STRIP, 6*1*10, GL_UNSIGNED_INT, nullptr);


    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("***  GL errors standing in your way\n");
    }

    // clear the binding to the default VAO
    glBindVertexArray(0);
}


void init_opengl_objects() {
    GLuint tempBufID;

    // inventory place card
    glGenVertexArrays(1, &inventory_cells_va);
    glBindVertexArray(inventory_cells_va);
    GLfloat *cells_data = new GLfloat[2 * 4*5*10];   // 2 coords, 4 vertexes to a square, 5 rows of 10 columns
    GLint *cells_indexes = new GLint[6*5*10];   // 6 edges to a square with zero width lines to next square, 5 rows of 10 columns

    for (int y=0; y<5; ++y) {
        for (int x=0; x<10; ++x) {
            cells_data[(x+y*10)*8+0] = 0.005f + 1.5f*(x-5)/10.0f;
            cells_data[(x+y*10)*8+1] = -y/10.0f*1.5f;

            cells_data[(x+y*10)*8+2] = 0.005f + 1.5f*(x-5)/10.0f+0.14;
            cells_data[(x+y*10)*8+3] = -y/10.0f*1.5f;

            cells_data[(x+y*10)*8+4] = 0.005f + 1.5f*(x-5)/10.0f;
            cells_data[(x+y*10)*8+5] = -y/10.0f*1.5f+0.14;

            cells_data[(x+y*10)*8+6] = 0.005f + 1.5f*(x-5)/10.0f + 0.14;
            cells_data[(x+y*10)*8+7] = -y/10.0f*1.5f + 0.14;
        }
    }

    for (int y=0; y<5; ++y) {
        for (int x=0; x<10; ++x) {
            cells_indexes[(x+y*10)*6+0] = (x+y*10)*4;
            cells_indexes[(x+y*10)*6+1] = (x+y*10)*4;
            cells_indexes[(x+y*10)*6+2] = (x+y*10)*4+1;
            cells_indexes[(x+y*10)*6+3] = (x+y*10)*4+2;
            cells_indexes[(x+y*10)*6+4] = (x+y*10)*4+3;
            cells_indexes[(x+y*10)*6+5] = (x+y*10)*4+3;
        }
    }

    glGenBuffers(1, &tempBufID);
    glBindBuffer(GL_ARRAY_BUFFER, tempBufID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*2*4*5*10, cells_data, GL_STATIC_DRAW);

    glGenBuffers(1, &tempBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempBufID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLint)*6*5*10, cells_indexes, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,  // attribute 0.
        2,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        2 * sizeof(GLfloat),  // stride
        (void*)0 // array buffer offset
    );
    glEnableVertexAttribArray(0);
    delete cells_data;
    delete cells_indexes;



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
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);


    glGenVertexArrays(1, &cube_id);
    glBindVertexArray(cube_id);
    GLfloat cube_data[8 * 3] = { 0,0,0, 0,0,1, 0,1,0, 0,1,1, 1,0,0, 1,0,1, 1,1,0, 1,1,1};
    GLint cube_indexes[5 * 2 + 3 + 6] = { 0,1, 4,5, 6,7, 2,3, 0,1,   5,3,7,  7,6,6,4,2,0 };

    GLuint cube_data_buf_id;
    glGenBuffers(1, &cube_data_buf_id);
    glBindBuffer(GL_ARRAY_BUFFER, cube_data_buf_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_data), cube_data, GL_STATIC_DRAW);

    glGenBuffers(1, &tempBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempBufID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indexes), cube_indexes, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,  // attribute 0.
        3,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        3 * sizeof(GLfloat),  // stride
        (void*)0 // array buffer offset
    );
    glEnableVertexAttribArray(0);



    GLint cube_outline_indexes[5 * 2 + 6] = { 0,4,6,2,0, 1,5,7,3,1, 5,4,6,7,3,2 };
    glGenVertexArrays(1, &cube_outline_id);
    glBindVertexArray(cube_outline_id);
    glBindBuffer(GL_ARRAY_BUFFER, cube_data_buf_id);
    glGenBuffers(1, &tempBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempBufID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_outline_indexes), cube_outline_indexes, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,  // attribute 0.
        3,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        3 * sizeof(GLfloat),  // stride
        (void*)0 // array buffer offset
    );
    glEnableVertexAttribArray(0);





// if roach
    // glGenVertexArrays(1, &roach_id);
    // glBindVertexArray(roach_id);

    // glGenBuffers(1, &tempBufID);
    // glBindBuffer(GL_ARRAY_BUFFER, tempBufID);
    // glBufferData(GL_ARRAY_BUFFER, sizeof_roach_data, roach_data, GL_STATIC_DRAW);

    // glGenBuffers(1, &tempBufID);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tempBufID);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_roach_indexes, roach_indexes, GL_STATIC_DRAW);

    // glVertexAttribPointer(
    //     0,  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    //     3,  // size
    //     GL_FLOAT,   // type
    //     GL_FALSE,   // normalized?
    //     3 * sizeof(GLfloat),  // stride
    //     (void*)0 // array buffer offset
    // );
    // glEnableVertexAttribArray(0);


#if 0
    glVertexAttribPointer(
        1,
        2,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        5 * sizeof(GLfloat),  // stride
        (void*)(3 * sizeof(GLfloat)) // array buffer offset
    );
    glEnableVertexAttribArray(1);
#endif


    // clear the binding to the default VAO
    glBindVertexArray(0);

    // clear the bound buffer array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    
    textureShader = new TextureShader("shaders/SimpleTextureShader.vert", "shaders/SimpleTextureShader.frag");
    colorShader = new FixedColorShader("shaders/FixedColorShader.vert", "shaders/FixedColorShader.frag");

    textureDict[1] = new Texture("textures/wall_0.jpg");
    textureDict[1024] = new Texture("textures/dirt_floor_0.png");
    monsterTex = new Texture("textures/dummy_0.jpg");
    crosshairsTex = new Texture("textures/crosshairs_0.png");

    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("***  GL errors standing in your way\n");
    }
}

//#############################################################################

#if 0
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
#endif

//#############################################################################
#if 0   // if roach

static void get_vertex(const char *buffer, GLfloat *a, GLfloat *b, GLfloat *c) {
    *a = atof(buffer) / 10;
    ++buffer;
    while (*buffer++ != ' ') ;
    *b = atof(buffer) / 10;
    while (*buffer++ != ' ') ;
    *c = atof(buffer) / 10;
}

static void get_face(const char *buffer, GLint *a, GLint *b, GLint *c, GLint *d) {
    *a = atoi(buffer) -1;
    ++buffer;
    while (*buffer++ != ' ') ;
    *b = atoi(buffer) -1;
    while (*buffer++ != ' ') ;
    *c = atof(buffer) -1;
    while (*buffer++ != ' ') ;
    *d = atof(buffer) -1;
}

void load_model(const char *file_name) {
    FILE *f = fopen(file_name, "r");

    char *buffer = new char[16*1024*1024];
    if (buffer == nullptr) {
        printf("*** weak memory - can not load model\n");
        return;
    }
    size_t bytes_read = fread(buffer, 1, 16*1024*1024, f);

    int vertex_count = 0;
    int face_count = 0;
    for (size_t i=0; i<bytes_read-2; ++i) {
        if (buffer[i] == 'v' && buffer[i+1] == ' ' && buffer[i+2] == ' ') {
            ++vertex_count;
        } else if (buffer[i] == 'f' && buffer[i+1] == ' ') {
            ++face_count;
        }
    }
    printf("vertex count:  %d\n", vertex_count);
    printf("face count:  %d\n", face_count);

    sizeof_roach_data = vertex_count * 3 * sizeof(GLfloat);
    sizeof_roach_indexes = face_count * 4 * sizeof(GLint);
    count_roach_indexes = face_count*4;

    roach_data = new GLfloat[vertex_count * 3];
    roach_indexes = new GLint[face_count * 4];

#if 0
    if (roach_data == nullptr) {
        printf("*** no roach data for you\n");
        return;
    }
    if (roach_indexes == nullptr) {
        printf("*** no roach indexes for you\n");
        return;
    }
#endif

    vertex_count = 0;
    face_count = 0;
    for (size_t i=0; i<bytes_read-2; ++i) {
        if (buffer[i] == 'v' && buffer[i+1] == ' ' && buffer[i+2] == ' ') {
            get_vertex(&buffer[i+3], &roach_data[vertex_count*3], &roach_data[vertex_count*3+1], &roach_data[vertex_count*3+2]);
            ++vertex_count;
        } else if (buffer[i] == 'f' && buffer[i+1] == ' ') {
            get_face(&buffer[i+2], &roach_indexes[face_count*4], &roach_indexes[face_count*4+1], &roach_indexes[face_count*4+2], &roach_indexes[face_count*4+3]);
            ++face_count;
        }
    }
     
#if 0
    for(int i=0; i<100; ++i) {
        printf("%f, ", roach_data[i]);
    }
    for(int i=0; i<100; ++i) {
        printf("%d, ", roach_indexes[i]);
    }
#endif

    delete buffer;
    fclose(f);
}
#endif

//#############################################################################

void update_object_locations(Player &player, const Chunk &chunk, float *look, double time_delta) {
    player.update_pos(chunk, look, time_delta);

    missile.update(time_delta);
    falling_cube.update(time_delta, chunk);
}


void fill_example_chunk(Chunk &chunk) {
    for (int x=0; x<16; ++x) {
        for (int y=0; y<16; ++y) {
            chunk.grid[0][y][x] = 1;
        }
    }

    chunk.grid[0][5][5] = 0;
    chunk.grid[2][5][5] = 1;
}

int main(void) {
    Player player;
    MovementInputState player_movement_state = {};

    Chunk   chunk={};
    fill_example_chunk(chunk);


    // load_model("models/15919_Cockroach_v1.obj");

    GLFWwindow *window = init_glfw();
    init_opengl_objects();
    // init_game_objects();

    float ratio;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;
    glViewport(0, 0, width, height);
    Scene scene(ratio);

    // number of seconds since initialization
    double time_start = glfwGetTime();
    double time_prev = time_start;
    while (!glfwWindowShouldClose(window)) {
        float pos[3];
        float look[3];
        player.get_eye_pos(pos);
        player.get_eye_look(look);  // get eye so we know which direction forward is
        scene.render(chunk, pos, look);


        glfwSwapBuffers(window);
        // the above waits for one screen update, then returns for us to fill in the back buffer
        // so our event processing and in game time is tied to vsync... which isn't what we really want

        // get the wrong sequence of keys with glfw, unless called multiple times
        // redcuing this to one call, after repeat happens, get an extra Press on Release
        glfwPollEvents();
        glfwPollEvents();
        glfwPollEvents();
        glfwPollEvents();

        double time = glfwGetTime();
        double time_delta = 100*(time - time_prev); // convert this to number of ticks?
        time_prev = time;

        process_keys(captured_keyboard_inputs, player_movement_state);
        captured_keyboard_inputs.clear();
        player.update_movement(player_movement_state);

        player.update_gaze(captured_mouse_movements);
        captured_mouse_movements.clear();
        
        player.update_eye_pos(time_delta);
        update_object_locations(player, chunk, look, time_delta);
    }
    shutdown_glfw(window);
}
