#include <iostream>
#include <cmath>
#include <queue>

class Player {

public:
    Player() : pos_x_(123456789.1), pos_y_(0) {}
    void render_visible_tiles();

private:
    double pos_x_, pos_y_;
};


void Player::render_visible_tiles() {
    long tile_x = (pos_x_ / 16);
    long tile_y = (pos_y_ / 16);
    float x_in_tile = (float)fmod(pos_x_, 16);
    float y_in_tile = (float)fmod(pos_y_, 16);


    std::cout << "Tiles player can see:  " << std::endl;
    std::cout << "  " << tile_x << ", " << tile_y;
    std::cout << "  standing at " << x_in_tile << ", " << y_in_tile << std::endl;
}


struct triangle {
    triangle(int p0_x, int p0_y, int p1_x, int p1_y, int p2_x, int p2_y) : p0_x_(p0_x),p0_y_(p0_y),p1_x_(p1_x),p1_y_(p1_y),p2_x_(p2_x),p2_y_(p2_y) {}
    int p0_x_, p0_y_;
    int p1_x_, p1_y_;
    int p2_x_, p2_y_;
};


void dump_height_map(const float *height_map, int rows) {
    for (int y=0; y<rows; ++y) {
        for (int x=0; x<rows; ++x) {
            std::cout << height_map[y*rows + x] << ", ";
        }
        std::cout << std::endl;
    }
}

void fracture_tile(int rows) {
    float height_map[rows+1][rows+1] = {};
    height_map[rows/2][rows/2] = 1.0;
    height_map[0][rows/2] = 0.5;
    height_map[rows/2][0] = 0.5;
    height_map[rows/2][rows] = 0.5;
    height_map[rows][rows/2] = 0.5;


   std::queue<triangle> q; 
    
    q.push( triangle(0,0,rows,rows,rows,0) );
    q.push( triangle(0,0,rows,rows,0,rows) );

    while (!q.empty()) {
        const triangle& p = q.front();

        int new_p0_x = (p.p0_x_ + p.p1_x_)/2;
        int new_p0_y = (p.p0_y_ + p.p1_y_)/2;
        int new_p1_x = (p.p0_x_ + p.p2_x_)/2;
        int new_p1_y = (p.p0_y_ + p.p2_y_)/2;
        int new_p2_x = (p.p1_x_ + p.p2_x_)/2;
        int new_p2_y = (p.p1_y_ + p.p2_y_)/2;

        if (!( (new_p0_x == new_p1_x && new_p0_y == new_p1_y) || (new_p0_x == new_p2_x && new_p0_y == new_p2_y) || (new_p1_x == new_p2_x && new_p1_y == new_p2_y))) {
            if (height_map[new_p0_y][new_p0_x] == 0) {
                height_map[new_p0_y][new_p0_x] = (height_map[p.p0_y_][p.p0_x_] + height_map[p.p1_y_][p.p1_x_]) / 2;
            }
            if (height_map[new_p1_y][new_p1_x] == 0) {
                height_map[new_p1_y][new_p1_x] = (height_map[p.p0_y_][p.p0_x_] + height_map[p.p2_y_][p.p2_x_]) / 2;
            }
            if (height_map[new_p2_y][new_p2_x] == 0) {
                height_map[new_p2_y][new_p2_x] = (height_map[p.p1_y_][p.p1_x_] + height_map[p.p2_y_][p.p2_x_]) / 2;
            }
            q.push( triangle(new_p0_x, new_p0_y, new_p1_x, new_p1_y, p.p0_x_, p.p0_y_) );
            q.push( triangle(new_p0_x, new_p0_y, p.p1_x_, p.p1_y_, new_p2_x, new_p2_y) );
            q.push( triangle(p.p2_x_, p.p2_y_, new_p1_x, new_p1_y, new_p2_x, new_p2_y) );
            q.push( triangle(new_p0_x, new_p0_y, new_p1_x, new_p1_y, new_p2_x, new_p2_y) );
        }

        q.pop();
    }

    dump_height_map((float*)height_map, rows+1);
}


int main() {
    Player p;

    p.render_visible_tiles();

    fracture_tile(16);
}
