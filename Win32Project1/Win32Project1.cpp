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

#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#define max(a, b) ( (a) < (b) ? (b) : (a) )
#define min(a, b) ( (a) > (b) ? (b) : (a) )

const GLfloat PLAYER_HEIGHT=1.5f;

enum PlayerMovementState {
    OnGround,
    Falling,
    Flying,
    Swimming,
};

GLfloat upDownV = 0;
GLfloat leftRightV = 0; //left neg, right pos
GLfloat forwardV = 0, rotationV = 0;
GLfloat pos[3] = {0,0,1.2f};
GLfloat eyePos[3] = {0,0,1.2f+PLAYER_HEIGHT};
GLfloat facing = 45 * 3.14159f / 180;
GLfloat facingUpDown = -45 * 3.14159f / 180;
static PlayerMovementState playerState = Falling;
bool gPlayerCanMove=true;

const GLfloat GRAVITY = -0.001f;
const GLfloat MAX_STEP_HEIGHT = 0.55f;


#define DEPTH 15
#define ROWS 15
#define COLS 15

float heightMap[DEPTH][COLS][ROWS] = {};
#if 0
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0} };
#endif

#define sqr(x) (x*x)
void normalize(GLfloat *vec) {
    float d = sqrt(sqr(vec[0]) + sqr(vec[1]) + sqr(vec[2]));
    vec[0] /= d;
    vec[1] /= d;
    vec[2] /= d;
}

GLfloat dot(const GLfloat *a, const GLfloat *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// c = a cross b
void cross(const GLfloat *a, const GLfloat *b, GLfloat *c) {
    c[0] = (a[1] * b[2]) - (a[2] * b[1]);
    c[1] = (a[2] * b[0]) - (a[0] * b[2]);
    c[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

void matmul(const GLfloat *a, const GLfloat *b, GLfloat *c) {
    for (int row=0; row<4; ++row) {
        for (int col=0; col<4; ++col) {
            GLfloat sum = 0;
            for (int i=0; i<4; ++i) {
                sum += a[row+4*i] * b[i+4*col];
            }
            c[row+4*col] = sum;
        }
    }
}

void lookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx, GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy, GLfloat upz, GLfloat *m) {
    GLfloat Z[3] = {eyex - centerx, eyey - centery, eyez - centerz};
    normalize(Z);
    GLfloat X[3];
    GLfloat Y[3] = {upx, upy, upz};
    cross(Y, Z, X);
    cross(Z, X, Y);
    normalize(X);
    normalize(Y);

    GLfloat eye[3] = {eyex, eyey, eyez};
    m[0] = X[0];
    m[4] = X[1];
    m[8] = X[2];
    m[12] = -dot(X, eye);

    m[1] = Y[0];
    m[5] = Y[1];
    m[9] = Y[2];
    m[13] = -dot(Y, eye);

    m[2] = Z[0];
    m[6] = Z[1];
    m[10] = Z[2];
    m[14] = -dot(Z, eye);

    m[3] = 0;
    m[7] = 0;
    m[11] = 0;
    m[15] = 1;
}

// redefine near/far  as n/f, due to Windows headers
void perspective(GLfloat angle, GLfloat ratio, GLfloat n, GLfloat f, float *p) {
    GLfloat e = 1 / tanf(angle * 3.14159f / 180 / 2);
    p[0] = e;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;

    p[4] = 0;
    p[5] = e * ratio;
    p[6] = 0;
    p[7] = 0;

    p[8] = 0;
    p[9] = 0;
    p[10] = -(f + n) / (f - n);
    p[11] = -1;

    p[12] = 0;
    p[13] = 0;
    p[14] = -2 * f * n / (f -n);
    p[15] = 0;
}

void update_pos_onground(const GLfloat* look) {
    // estimate a new position for the player
    // newpos is based on:
    //   if player has a velocity, reduce it by the blocktype
    // does newpos put player into a new tile?  if so:
    //   would he now be falling?
    //   is the height difference acceptable?
    // if still onground, update velocity by keys pressed
    // accept newpos if it is valid

    GLfloat Z[3] = { look[0] - pos[0], look[1] - pos[1], 0 }; //, look[2] - pos[2]};
    normalize(Z);
    GLfloat newPos[3];
    newPos[0] = pos[0] + Z[0] * forwardV - Z[1] * leftRightV;
    newPos[1] = pos[1] + Z[1] * forwardV + Z[0] * leftRightV;
    newPos[2] = pos[2] + upDownV;

    bool validMove = false;
    // check falling off grid
    if ((newPos[0] <= 0) || (newPos[0] >= 10) || (newPos[1] <= 0) || (newPos[1] >= 10)) {
        //playerState = Falling;
        //validMove = true;
    } else {
        //printf("%d, %d\n", (int)(newPos[0]*5 + 5), (int)(newPos[1]*5 + 5));
        float groundHeight = heightMap[0][(int)(newPos[0])][(int)(newPos[1])];
        float myHeight = newPos[2];
        if (groundHeight - myHeight <= MAX_STEP_HEIGHT) {
            validMove = true;
            if (newPos[2] < groundHeight) {
                newPos[2] = groundHeight;
                upDownV = 0;
                playerState = OnGround;
            } else if (newPos[2] > groundHeight) {
                playerState = Falling;
            }
        }
    }


    if (validMove) {
        pos[0] = newPos[0];
        pos[1] = newPos[1];
        pos[2] = newPos[2];
    }
    facing += rotationV;        // need to include a timestamp to handle fps correctly
}

void update_pos_falling(const GLfloat* look) {
    upDownV += GRAVITY;

    GLfloat Z[3] = { look[0] - pos[0], look[1] - pos[1], 0 }; //, look[2] - pos[2]};
    normalize(Z);
    GLfloat newPos[3];
    newPos[0] = pos[0] + Z[0] * forwardV - Z[1] * leftRightV;
    newPos[1] = pos[1] + Z[1] * forwardV + Z[0] * leftRightV;
    newPos[2] = pos[2] + upDownV;

    float groundHeight = heightMap[0][(int)(newPos[0])][(int)(newPos[1])];
    if (newPos[2] < groundHeight) {
        newPos[2] = groundHeight;
        upDownV = 0;
        playerState = OnGround;
    }

    bool validMove = true;
    if (validMove) {
        pos[0] = newPos[0];
        pos[1] = newPos[1];
        pos[2] = newPos[2];
    }
    facing += rotationV;        // need to include a timestamp to handle fps correctly
}

/*
*   First calculate where we are, then the direction we are looking from that position
*/
void update_pos(const GLfloat* look) {
    switch (playerState) {
        case OnGround:
            update_pos_onground(look);
            break;
        case Falling:
            update_pos_falling(look);
            break;
        default:
            break;
    }
}


void get_eye(GLfloat *look) {
    look[0] = pos[0] + cosf(facingUpDown) * cosf(facing);
    look[1] = pos[1] + cosf(facingUpDown) * sinf(facing);
    look[2] = pos[2] + sinf(facingUpDown) + PLAYER_HEIGHT;
}

static void update_facing(GLfloat elevationDelta, GLfloat rotationDelta) {
    facingUpDown -= elevationDelta;
    facingUpDown = max(-3.14159f / 2, facingUpDown);
    facingUpDown = min(3.14159f / 2, facingUpDown);

    facing -= rotationDelta;
}


void update_player_motion(int forwardMotion, int sidewaysMotion, bool tryingToJump) {
    if (gPlayerCanMove) {

        if (tryingToJump) {
            if (playerState == OnGround) {
                playerState = Falling;
                upDownV += 0.05f;
            }
        }
    }

    // with this outside of the gPlayerCanMove check, if a key is pressed, planar motion stops
    // if the player is in air, that motion will continue.  Might change how motion works in the future.
    if (forwardMotion == 0 && sidewaysMotion == 0) {
        forwardV = 0;
        leftRightV = 0;
    } else {
        // normalize so that our speed doesn't increase with diagonal movement
        forwardV = forwardMotion * 0.02f;
        leftRightV = sidewaysMotion * 0.02f;
        float d = 0.035f / (sqrt(sqr(forwardV) + sqr(leftRightV)));
        forwardV *= d;
        leftRightV *= d;
    }
}


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
    int forwardMotion=0;
    int sidewaysMotion=0;
    bool tryingToJump = false;

    bool forwardKey = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool backKey = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool leftKey = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool rightKey = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

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

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        tryingToJump = true;
    }

    update_player_motion(forwardMotion, sidewaysMotion, tryingToJump);
}

static double xposPrev=0, yposPrev = 0;
const GLfloat SCALED_MOUSE_MOVEMENT = 0.001f;
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    GLfloat elevationDelta = (GLfloat)(ypos - yposPrev) * SCALED_MOUSE_MOVEMENT;
    GLfloat rotationDelta = (GLfloat)(xpos - xposPrev) * SCALED_MOUSE_MOVEMENT;
    yposPrev = ypos;
    xposPrev = xpos;
    update_facing(elevationDelta, rotationDelta);
}

static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}


/*======================================================================*
 * main()
 *======================================================================*/

int main( void )
{
    heightMap[0][1][1] = 1.0;


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

#if 0
    srand((unsigned int)time(NULL));
    for (int j=0; j<10; ++j) {
        for (int i=0; i<10; ++i) {
            //heightMap[i][j] = rand() / (float)RAND_MAX;
            heightMap[0][i][j] = (sinf((i+j)/5.0f) + 1)/2.0f;
        }
    }
#endif



    GLuint GridArrayID;
    glGenVertexArrays(1, &GridArrayID);
    glBindVertexArray(GridArrayID);

    // 3 values per vertex, 2 vertex per line, 2 sets of 11 lines
    GLfloat g_vertex_buffer_data[400*2*3];
    for (int j=0; j<10; j++) {
        for (int i=0; i<10; ++i) {
            g_vertex_buffer_data[240*j + i*24] = (GLfloat)i;
            g_vertex_buffer_data[240*j + i*24+1] = (GLfloat)j;
            g_vertex_buffer_data[240*j + i*24+2] = heightMap[0][i][j];
            g_vertex_buffer_data[240*j + i*24+3] = (GLfloat)i+1;
            g_vertex_buffer_data[240*j + i*24+4] = (GLfloat)j;
            g_vertex_buffer_data[240*j + i*24+5] = heightMap[0][i][j];

            g_vertex_buffer_data[240*j + i*24+6] = (GLfloat)i+1;
            g_vertex_buffer_data[240*j + i*24+7] = (GLfloat)j;
            g_vertex_buffer_data[240*j + i*24+8] = heightMap[0][i][j];
            g_vertex_buffer_data[240*j + i*24+9] = (GLfloat)i+1;
            g_vertex_buffer_data[240*j + i*24+10] = (GLfloat)j+1;
            g_vertex_buffer_data[240*j + i*24+11] = heightMap[0][i][j];

            g_vertex_buffer_data[240*j + i*24+12] = (GLfloat)i+1;
            g_vertex_buffer_data[240*j + i*24+13] = (GLfloat)j+1;
            g_vertex_buffer_data[240*j + i*24+14] = heightMap[0][i][j];
            g_vertex_buffer_data[240*j + i*24+15] = (GLfloat)i;
            g_vertex_buffer_data[240*j + i*24+16] = (GLfloat)j+1;
            g_vertex_buffer_data[240*j + i*24+17] = heightMap[0][i][j];

            g_vertex_buffer_data[240*j + i*24+18] = (GLfloat)i;
            g_vertex_buffer_data[240*j + i*24+19] = (GLfloat)j+1;
            g_vertex_buffer_data[240*j + i*24+20] = heightMap[0][i][j];
            g_vertex_buffer_data[240*j + i*24+21] = (GLfloat)i;
            g_vertex_buffer_data[240*j + i*24+22] = (GLfloat)j;
            g_vertex_buffer_data[240*j + i*24+23] = heightMap[0][i][j];
        }
    }

    // This will identify our vertex buffer
    GLuint vertexbuffer;
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        0,  // stride
        (void*)0 // array buffer offset
    );


    // 2 values per vertex, 2 vertex per line, 2 sets of 11 lines
    GLfloat textureCoordData[400*2*2];
    for (int j=0; j<10; j++) {
        for (int i=0; i<10; ++i) {
            textureCoordData[160*j + i*16] = 0;
            textureCoordData[160*j + i*16+1] = 0;
            textureCoordData[160*j + i*16+2] = 1;
            textureCoordData[160*j + i*16+3] = 0;

            textureCoordData[160*j + i*16+4] = 1;
            textureCoordData[160*j + i*16+5] = 0;
            textureCoordData[160*j + i*16+6] = 1;
            textureCoordData[160*j + i*16+7] = 1;

            textureCoordData[160*j + i*16+8] = 1;
            textureCoordData[160*j + i*16+9] = 1;
            textureCoordData[160*j + i*16+10] = 0;
            textureCoordData[160*j + i*16+11] = 1;

            textureCoordData[160*j + i*16+12] = 0;
            textureCoordData[160*j + i*16+13] = 1;
            textureCoordData[160*j + i*16+14] = 0;
            textureCoordData[160*j + i*16+15] = 0;
        }
    }


    GLuint textureCoordbuffer;
    glGenBuffers(1, &textureCoordbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textureCoordbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordData), textureCoordData, GL_STATIC_DRAW);
    glVertexAttribPointer(
        1,
        2,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        0,  // stride
        (void*)0 // array buffer offset
    );



    GLuint PegArrayID;
    glGenVertexArrays(1, &PegArrayID);
    glBindVertexArray(PegArrayID);

    // a box of 12 lines, 2 vertex per line, 3 values per vertex
    GLfloat g_peg_buffer_data[6];
    g_peg_buffer_data[0] = 0;
    g_peg_buffer_data[1] = -1;
    g_peg_buffer_data[2] = 0.5f;

    g_peg_buffer_data[3] = 0;
    g_peg_buffer_data[4] = 1;
    g_peg_buffer_data[5] = 0.5f;

    // This will identify our vertex buffer
    GLuint pegVertexbuffer;
    // Generate 1 buffer, put the resulting identifier in gridVertexbuffer
    glGenBuffers(1, &pegVertexbuffer);
    // The following commands will talk about our 'gridVertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, pegVertexbuffer);
    // Give our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_peg_buffer_data), g_peg_buffer_data, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,  // size
        GL_FLOAT,   // type
        GL_FALSE,   // normalized?
        0,  // stride
        (void*)0 // array buffer offset
    );

    
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);


    glfwSwapInterval( 1 );
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetCursorPos(window, &xposPrev, &yposPrev);

    TextureShader textureShader = TextureShader( "..\\shaders\\SimpleTextureShader.vert", "..\\shaders\\SimpleTextureShader.frag" );
    textureShader.use();

    Texture grass1Tex = Texture(std::wstring(L"..\\textures\\Grass_3.png"));

    FixedColorShader fixedColorShader = FixedColorShader( "..\\shaders\\FixedColorShader.vert", "..\\shaders\\FixedColorShader.frag" );
    fixedColorShader.use();

    GLfloat identity[] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    GLfloat view[16], projection[16];
    GLfloat red[] = {1,0,0};
    GLfloat green[] = {0,1,0};
    GLfloat black[] = {0,0,0};
    GLfloat grey[] = {0.5f,0.5f,0.5f};

   /* Main loop */
    int t = 0;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float ratio;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;
        glViewport(0, 0, width, height);
        GLfloat MVP[16];
        GLfloat look[3];
        get_eye(look);  // get eye so we know which direction forward is
        update_pos(look);
        get_eye(look);  // get the eye in its new position, after movement has happened
        lookAt(pos[0],pos[1],pos[2]+PLAYER_HEIGHT,  look[0], look[1], look[2],  0,0,1, view);
        perspective(45, ratio, 0.1f, 100.0f, projection);
        matmul(projection, view, MVP);

        // Draw the grid of stuff!
        textureShader.use();
        grass1Tex.bind();
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(textureShader.samplerID, 0);
        glUniformMatrix4fv(textureShader.matrixID, 1, GL_FALSE, MVP);
        glBindVertexArray(GridArrayID);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_QUADS, 0, 800); // 2 vertex per line, 4 lines per square, 100 squares
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // draw a spinning red line, just to show we are alive :)
        fixedColorShader.use();
        #define SCALE  0.03f
        GLfloat model[16] = {cosf(t*SCALE),sinf(t*SCALE),0,0, -sinf(t*SCALE),cosf(t*SCALE),0,0, 0,0,1,0, 0.9f,0.9f,0,1};
        GLfloat lineMVP[16];
        matmul(MVP, model, lineMVP);
        glUniformMatrix4fv(fixedColorShader.matrixID, 1, GL_FALSE, lineMVP);
        glBindVertexArray(PegArrayID);
        glUniform3fv(fixedColorShader.colorID, 1, red);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_LINES, 0, 2); // 2 vertex per line
        glDisableVertexAttribArray(0);

#if 0
        // draw a grey box always in front
        glDisable(GL_DEPTH_TEST);
        //glDisable(GL_CULL_FACE);
        //glDisable(GL_TEXTURE_2D);
        //glDisable(GL_LIGHTING);

        const GLfloat quadVertices[] = { -0.5f, -0.5f, 0.0f, 
            0.5f, -0.5f, 0.0f, 
            0.5f,-0.75f, 0.0f,
            -0.5f,-0.75f, 0.0f
        }; 
        fixedColorShader.use();
        glBindVertexArray(0);
        glUniformMatrix4fv(fixedColorShader.matrixID, 1, GL_FALSE, identity);
        glUniform3fv(fixedColorShader.colorID, 1, grey);
        glVertexPointer(3, GL_FLOAT, 0, quadVertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDrawArrays(GL_QUADS, 0, 4);
        glDisableClientState(GL_VERTEX_ARRAY);

        glEnable(GL_DEPTH_TEST);
        //glEnable(GL_CULL_FACE);
        //glEnable(GL_TEXTURE_2D);
        //glEnable(GL_LIGHTING);
#endif

        glfwSwapBuffers(window);
        glfwPollEvents();
        t += 1;
   }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

