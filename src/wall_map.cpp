#include <vector>

class WallMap {
public:
    WallMap(int width, int height) : width(width), height(height) {
        // Initialize the map with `false` values (no wall)
        wall_map.resize(width, std::vector<bool>(height, false));
    }

    void addWall(int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            wall_map[x][y] = true;
        }
    }

    bool isWall(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return wall_map[x][y];
        }
        return false; // Out of bounds treated as non-wall
    }

private:
    int width;
    int height;
    std::vector<std::vector<bool>> wall_map;
};
