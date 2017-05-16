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

int main(void) {
    GameView window;
#if 0
    init_opengl_objects();
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
        window.swapBuffers();
        window.pollEvents();
//        update_object_locations(look);
    }

    return 0;
}

