#include <cstdint>


const float GRAVITY = -0.001f;
const float MAX_STEP_HEIGHT = 0.55f;

static inline bool is_blocking(uint32_t gridElement) {
    if (gridElement < 1024) return true;
    return false;
}

