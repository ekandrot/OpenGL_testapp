#pragma once

#include <math.h>

//#############################################################################

template <class T>
inline T sqr(const T &x) {return x * x;}

//#############################################################################

inline void normalize(float *vec) {
    float d = sqrt(sqr(vec[0]) + sqr(vec[1]) + sqr(vec[2]));
    vec[0] /= d;
    vec[1] /= d;
    vec[2] /= d;
}

inline float dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// c = a cross b
inline void cross(const float *a, const float *b, float *c) {
    c[0] = (a[1] * b[2]) - (a[2] * b[1]);
    c[1] = (a[2] * b[0]) - (a[0] * b[2]);
    c[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

inline void matmul(const float *a, const float *b, float *c) {
    for (int row = 0; row<4; ++row) {
        for (int col = 0; col<4; ++col) {
            float sum = 0;
            for (int i = 0; i<4; ++i) {
                sum += a[row + 4 * i] * b[i + 4 * col];
            }
            c[row + 4 * col] = sum;
        }
    }
}

inline void lookAt(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz, float *m) {
    float Z[3] = { eyex - centerx, eyey - centery, eyez - centerz };
    normalize(Z);
    float X[3];
    float Y[3] = { upx, upy, upz };
    cross(Y, Z, X);
    cross(Z, X, Y);
    normalize(X);
    normalize(Y);

    float eye[3] = { eyex, eyey, eyez };
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
inline void perspective(float angle, float ratio, float n, float f, float *p) {
    float e = 1 / tanf(angle * 3.14159f / 180 / 2);
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
    p[14] = -2 * f * n / (f - n);
    p[15] = 0;
}

//#############################################################################

