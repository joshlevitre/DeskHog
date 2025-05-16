#include "UltimaGame.h"

UltimaGame::UltimaGame() : player_x(MAP_WIDTH / 2), player_y(MAP_HEIGHT / 2) {
    initializeMap();
    initializeStats(); // Initialize player stats
}

void UltimaGame::initializeStats() {
    hp = 100;
    max_hp = 100;
    level = 1;
    xp = 0;
}

void UltimaGame::initializeMap() {
    game_map.clear();
    // Create a simple map with a border of '#' and '.' for empty space
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        String row = "";
        for (int j = 0; j < MAP_WIDTH; ++j) {
            if (i == 0 || i == MAP_HEIGHT - 1 || j == 0 || j == MAP_WIDTH - 1) {
                row += "#"; // Keep walls as '#'
            } else {
                // Add some variety, like water
                if (i > MAP_HEIGHT / 2 && j > MAP_WIDTH / 2 && (i+j) % 4 == 0) {
                    row += "~"; // Water
                } else {
                    row += "."; // Floor
                }
            }
        }
        game_map.push_back(row);
    }
    // Example: Add some obstacles or features
    game_map[5][5] = 'T'; // A tree
    game_map[MAP_HEIGHT / 2][MAP_WIDTH / 3] = 'R'; // A rock
    game_map[3][3] = '~'; // Small pond
    game_map[3][4] = '~';
    game_map[4][3] = '~';
}

void UltimaGame::movePlayer(int dx, int dy) {
    int new_x = player_x + dx;
    int new_y = player_y + dy;

    // Basic boundary and collision check (can be expanded)
    if (new_x >= 0 && new_x < MAP_WIDTH && 
        new_y >= 0 && new_y < MAP_HEIGHT &&
        game_map[new_y][new_x] != '#' &&  // Cannot move into walls
        game_map[new_y][new_x] != '~') { // Cannot move into water (for now)
        player_x = new_x;
        player_y = new_y;
    }
}

String UltimaGame::renderView() {
    String view_str = "";
    // Calculate the top-left corner of the view relative to the map
    int view_start_x = player_x - VIEW_WIDTH / 2;
    int view_start_y = player_y - VIEW_HEIGHT / 2;

    for (int y_offset = 0; y_offset < VIEW_HEIGHT; ++y_offset) {
        for (int x_offset = 0; x_offset < VIEW_WIDTH; ++x_offset) {
            int map_render_x = view_start_x + x_offset;
            int map_render_y = view_start_y + y_offset;

            if (map_render_x == player_x && map_render_y == player_y) {
                view_str += "@";
            } else if (map_render_x >= 0 && map_render_x < MAP_WIDTH &&
                       map_render_y >= 0 && map_render_y < MAP_HEIGHT) {
                char tile = game_map[map_render_y][map_render_x];
                view_str += tile; // Simply append the tile character
            } else {
                view_str += ' '; // Out of map bounds, show empty space
            }
        }
        if (y_offset < VIEW_HEIGHT - 1) {
             view_str += "\n"; // Newline for each row except the last
        }
    }
    return view_str;
}

String UltimaGame::searchCurrentTile() {
    char tile_char = game_map[player_y][player_x];
    switch (tile_char) {
        case 'T':
            return "You see a sturdy tree.";
        case 'R':
            return "You see a large rock.";
        case '#':
            return "A solid wall blocks your way.";
        case '~':
            return "Water. Looks deep.";
        case '.':
            return "Clear ground. Nothing special."; // This was the example you gave
        default:
            return "An unknown object.";
    }
}

String UltimaGame::getFormattedStats() {
    String stats_str = "LVL: " + String(level) + "\n";
    stats_str += "HP:  " + String(hp) + "/" + String(max_hp) + "\n";
    stats_str += "XP:  " + String(xp);
    return stats_str;
} 