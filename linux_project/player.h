#pragma once
#include <algorithm>
#include <vector>

#include <GLFW/glfw3.h>

//#############################################################################

enum MovementState {
    OnGround,
    Falling,
    Flying,
    Swimming,
    God,
};


struct MovementInputState {
    bool _forwardKey;
    bool _backwardKey;
    bool _leftKey;
    bool _rightKey;
    bool _jumpKey;
    bool _diveKey;
    bool _runningKey;
};


//#############################################################################

struct Player {

    Player();

    void update_eye_pos(double time_delta);
    void get_eye_look(float *look) const;
    void get_eye_pos(float *look) const;

    void update_gaze(const std::vector<std::pair<float, float> > &mouse_movements);
    void update_movement(const MovementInputState &movementInputState);
    void update_velocities(int forwardMotion, int sidewaysMotion, int upMotion);
    void update_pos(const GLfloat* look, double time_delta);

    void update_pos_onground(const GLfloat* look, double time_delta);
    void update_pos_falling(const GLfloat* look, double time_delta);
    void update_pos_flying(const GLfloat* look, double time_delta);
    
    // position and view within the world
    bool _crouching = false;
    bool _running = false;
    float _eyeHeight = 1.5f;
    float _pos[3] = { 1,1,0 };
    float _facing = 45 * 3.14159f / 180;    // radians left/right around player axis
    float _facingUpDown = 0;//-15 * 3.14159f / 180;    // radians up/down with player eyes as level zero

                                                   // velocities
    float _upV = 0; // negative is down
    float _rightV = 0;  // negative is left
    float _forwardV = 0; // negative is backwards
    MovementState _movementState = Flying;//OnGround;
};

//#############################################################################

