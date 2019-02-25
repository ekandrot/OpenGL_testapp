#include <cstdint>

struct Chunk {
    inline bool is_passible(int x, int y, int z);
    
    uint32_t grid[16][16][16];
};

