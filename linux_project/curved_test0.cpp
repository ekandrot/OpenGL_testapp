#include <stdio.h>
#include <ctime>
#include <cstdlib>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include "game_view.h"
#include <GL/glew.h>
#include "shaders.h"
#include "texture.h"
#include "maths.h"


//#############################################################################

GLuint CubeID;
GLuint SquareTexturedVA;
TextureShader *textureShader;
std::map<uint32_t, Texture*> textureDict;
Texture *monsterTex;
float ratio;

//#############################################################################

static void init_opengl_objects() {
    GLuint tempBufID;

    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);

    // setup SquareTexturedVA  VAO
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


    // setup CubeID VAO
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
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // clear the bound buffer array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    

    // load shaders
    textureShader = new TextureShader("shaders/SimpleTextureShader.vert", "shaders/SimpleTextureShader.frag");

    // load tectures
    textureDict[1] = new Texture("textures/wall_0.jpg");
    textureDict[1024] = new Texture("textures/dirt_floor_0.png");
    monsterTex = new Texture("textures/dummy_0.jpg");
}

//#############################################################################

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float MVP[16], view[16], projection[16];
    //lookAt(pos[0], pos[1], pos[2], look[0], look[1], look[2], 0, 0, 1, view);
    lookAt(2, 0, 0, 0, 0, 0, 0, 0, 1, view);
    perspective(45, ratio, 0.1f, 100.0f, projection);
    matmul(projection, view, MVP);

    glBindVertexArray(CubeID);
    textureShader->use();
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(textureShader->samplerID, 0);
    GLfloat model[16] = { 0.5,0,0,0, 0,0.5,0,0, 0,0,0.5,0, -2.25,0.25,0.25,1 };
    GLfloat squareMVP[16];
    matmul(MVP, model, squareMVP);
    glUniformMatrix4fv(textureShader->matrixID, 1, GL_FALSE, squareMVP);
    monsterTex->bind();
    glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, nullptr);
}

//#############################################################################

int main(void) {
    GameView window;
    int width, height;
    window.getFramebufferDim(width, height);
    glViewport(0, 0, width, height);
    ratio = width / (float)height;

    init_opengl_objects();
#if 0
    init_game_objects();

    float ratio;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;
    glViewport(0, 0, width, height);
    Scene scene(ratio);
#endif
//    clock_t startTime = clock(); //Start timer
    while (!window.shouldQuit()) {
#if 0
        float pos[3];
        float look[3];
        player.update_eye_pos();
        player.get_eye_pos(pos);
        player.get_eye_look(look);  // get eye so we know which direction forward is
        scene.render(pos, look);
#endif
        renderScene();
        window.swapBuffers();
        window.pollEvents();
//        update_object_locations(look);
    }

    return 0;
}

