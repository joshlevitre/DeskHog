#include "UltimaGame.h"

// Define color strings for LVGL recoloring (LVGL v8+ syntax)
// Note: The space after the hex code is removed, it should be part of the text or not present if only one char.
// The format is "#RRGGBB TextToColor#"
const char* LV_COLOR_PLAYER_STR = "#FFFFFF"; // White
const char* LV_COLOR_WALL_STR   = "#808080"; // Grey
const char* LV_COLOR_FLOOR_STR  = "#228B22"; // ForestGreen
const char* LV_COLOR_TREE_STR   = "#32CD32"; // LimeGreen
const char* LV_COLOR_ROCK_STR   = "#A0522D"; // Sienna
const char* LV_COLOR_WATER_STR  = "#1E90FF"; // DodgerBlue
const char* LV_COLOR_DEFAULT_STR= "#303030"; // Dark Grey for empty space / out of bounds

UltimaGame::UltimaGame() : player_x(MAP_WIDTH / 2), player_y(MAP_HEIGHT / 2) {
    initializeMap();
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
    int view_start_x = player_x - VIEW_WIDTH / 2;
    int view_start_y = player_y - VIEW_HEIGHT / 2;

    for (int y_offset = 0; y_offset < VIEW_HEIGHT; ++y_offset) {
        for (int x_offset = 0; x_offset < VIEW_WIDTH; ++x_offset) {
            int map_render_x = view_start_x + x_offset;
            int map_render_y = view_start_y + y_offset;
            
            char current_char_on_map = ' '; 
            bool is_player_pos = (map_render_x == player_x && map_render_y == player_y);

            if (is_player_pos) {
                view_str += LV_COLOR_PLAYER_STR;
                view_str += "@";
                view_str += "#"; // End color marker
            } else if (map_render_x >= 0 && map_render_x < MAP_WIDTH &&
                       map_render_y >= 0 && map_render_y < MAP_HEIGHT) {
                current_char_on_map = game_map[map_render_y][map_render_x];
                String color_prefix;
                switch (current_char_on_map) {
                    case '#': color_prefix = LV_COLOR_WALL_STR; break;
                    case '.': color_prefix = LV_COLOR_FLOOR_STR; break;
                    case 'T': color_prefix = LV_COLOR_TREE_STR; break;
                    case 'R': color_prefix = LV_COLOR_ROCK_STR; break;
                    case '~': color_prefix = LV_COLOR_WATER_STR; break;
                    default: color_prefix = LV_COLOR_DEFAULT_STR; break;
                }
                view_str += color_prefix;
                view_str += current_char_on_map;
                view_str += "#"; // End color marker
            } else {
                view_str += LV_COLOR_DEFAULT_STR;
                view_str += " "; 
                view_str += "#"; // End color marker
            }
        }
        if (y_offset < VIEW_HEIGHT - 1) {
             view_str += "\n";
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
            return "Clear ground. Nothing special.";
        default:
            return "An unknown object.";
    }
} 