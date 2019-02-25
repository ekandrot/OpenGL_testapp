#include <cmath>

#include "maths.h"
#include "constants.h"
#include "player.h"


void Player::update_pos_onground(const GLfloat* look, double time_delta) {
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
    newPos[0] = _pos[0] + (Z[0] * _forwardV - Z[1] * _rightV) * time_delta;
    newPos[1] = _pos[1] + (Z[1] * _forwardV + Z[0] * _rightV) * time_delta;
    newPos[2] = _pos[2] + _upV * time_delta;

    bool validMove = false;
    // check falling off grid

    // the indexes of where we are on the grid
    int ix = (int)_pos[0];
    int iy = (int)_pos[1];

    // check for walls where we are, and prevent moving through them
    // if (is_blocking(grid[iy][ix-1])) {
    //     newPos[0] = std::max(newPos[0], ix + 0.15f);
    // }
    // if (is_blocking(grid[iy][ix+1])) {
    //     newPos[0] = std::min(newPos[0], ix + 0.85f);
    // }
    // if (is_blocking(grid[iy-1][ix])) {
    //     newPos[1] = std::max(newPos[1], iy + 0.15f);
    // }
    // if (is_blocking(grid[iy+1][ix])) {
    //     newPos[1] = std::min(newPos[1], iy + 0.85f);
    // }

    // make sure we stay within the grid, overall
    newPos[0] = std::max(0.15f, std::min(9.85f, newPos[0]));
    newPos[1] = std::max(0.15f, std::min(9.85f, newPos[1]));

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

void Player::update_pos_falling(const GLfloat* look, double time_delta) {
    _upV += GRAVITY * time_delta;

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
    // if (is_blocking(grid[iy][ix - 1])) {
    //     newPos[0] = std::max(newPos[0], ix + 0.15f);
    // }
    // if (is_blocking(grid[iy][ix + 1])) {
    //     newPos[0] = std::min(newPos[0], ix + 0.85f);
    // }
    // if (is_blocking(grid[iy - 1][ix])) {
    //     newPos[1] = std::max(newPos[1], iy + 0.15f);
    // }
    // if (is_blocking(grid[iy + 1][ix])) {
    //     newPos[1] = std::min(newPos[1], iy + 0.85f);
    // }

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

void Player::update_pos_flying(const GLfloat* look, double time_delta) {
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
void Player::update_pos(const GLfloat* look, double time_delta) {
    switch (_movementState) {
        case OnGround:
            update_pos_onground(look, time_delta);
            break;
        case Falling:
            update_pos_falling(look, time_delta);
            break;
        case Flying:
            update_pos_flying(look, time_delta);
            break;
        default:
            break;
    }
}


Player::Player() {
}

void Player::update_eye_pos(double time_delta) {
    if (_crouching) {
        if (_eyeHeight > 0.50) {
            _eyeHeight -= 0.05 * time_delta;
        } else {
            _eyeHeight = 0.50;
        }
    } else {
        if (_eyeHeight < 1.50) {
            _eyeHeight += 0.05 * time_delta;
        } else {
            _eyeHeight = 1.50;
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
    pos[2] = _pos[2] + _eyeHeight;
}

void Player::update_velocities(int forwardMotion, int sidewaysMotion, int upMotion) {
    const double maxSpeed = 2*(_crouching ? 0.008 : (_running ? 0.035 : 0.017));
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

void Player::update_gaze(const std::vector<std::pair<float, float> > &mouse_movements) {
    for (auto &pair : mouse_movements) {
        GLfloat elevationDelta = pair.first;
        GLfloat rotationDelta = pair.second;
        _facingUpDown -= elevationDelta;
        _facingUpDown = std::max(-3.14159f / 2, _facingUpDown);
        _facingUpDown = std::min(3.14159f / 2, _facingUpDown);

        _facing -= rotationDelta;
    }
}
