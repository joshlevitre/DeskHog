#include "UltimaGame.h"
#include <cstdlib> // For rand()
#include <ctime>   // For time() for seeding rand()
#include <string>  // For std::string

// Definition for the static const float member
const float UltimaGame::INITIAL_FLOOR_CHANCE = 0.45f;

UltimaGame::UltimaGame() : player_x(MAP_WIDTH / 2), player_y(MAP_HEIGHT / 2), current_level(GameLevel::OVERWORLD) {
    std::srand(std::time(0)); // Seed random number generator
    initializeOverworldMap();
    initializeStats(); // Initialize player stats
}

void UltimaGame::initializeStats() {
    hp = 10;
    max_hp = 10;
    level = 1;
    xp = 0;
}

void UltimaGame::initializeOverworldMap() { // Renamed from initializeMap
    game_map.clear();
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        String row = "";
        for (int j = 0; j < MAP_WIDTH; ++j) {
            if (i == 0 || i == MAP_HEIGHT - 1 || j == 0 || j == MAP_WIDTH - 1) {
                row += T_OVERWORLD_WALL; // Use defined constant for boundary
            } else {
                int r = rand() % 100; // Percentage roll
                if (r < 5) { // 5% chance for a Cave Entrance
                    row += T_CAVE_ENTRANCE;
                } else if (r < 10) { // 5% chance for an Oasis (total 10% for features)
                    row += T_OASIS;
                } else {
                    row += T_SAND;
                }
            }
        }
        game_map.push_back(row);
    }
    // Ensure player starting position is sand
    if (player_x > 0 && player_x < MAP_WIDTH -1 && player_y > 0 && player_y < MAP_HEIGHT -1) {
        game_map[player_y][player_x] = T_SAND;
    }
}

// New helper: Counts "alive" (floor) neighbors for a cell
int UltimaGame::countAliveNeighbors(const std::vector<String>& map_to_check, int x, int y, char wall_tile, char floor_tile) {
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue; // Skip the cell itself

            int neighbor_x = x + j;
            int neighbor_y = y + i;

            if (neighbor_x >= 0 && neighbor_x < MAP_WIDTH && neighbor_y >= 0 && neighbor_y < MAP_HEIGHT) {
                if (map_to_check[neighbor_y][neighbor_x] == floor_tile) {
                    count++;
                }
            } else {
                // Consider out-of-bounds as walls for border cells
                count++; // Effectively, this counts towards the wall condition if T_DUNGEON_WALL is the "alive" state for this interpretation
            }
        }
    }
    return count;
}

// New helper: Runs one iteration of cellular automata
void UltimaGame::runCellularAutomataIteration(std::vector<String>& map_to_smooth) {
    std::vector<String> next_map_state = map_to_smooth; // Create a copy to base changes on

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            // Keep outer border as walls
            if (y == 0 || y == MAP_HEIGHT - 1 || x == 0 || x == MAP_WIDTH - 1) {
                next_map_state[y][x] = T_DUNGEON_WALL;
                continue;
            }

            int alive_neighbors = countAliveNeighbors(map_to_smooth, x, y, T_DUNGEON_WALL, T_DUNGEON_FLOOR);

            if (map_to_smooth[y][x] == T_DUNGEON_FLOOR) {
                // A floor tile becomes a wall if it has less than (9 - WALL_THRESHOLD) floor neighbours
                // (i.e., WALL_THRESHOLD or more wall neighbours, assuming 8 neighbours total)
                // For typical CA, if floor neighbours < 4 (i.e. >4 wall neighbours), it dies (becomes wall)
                // If WALL_THRESHOLD = 5, means if floor_neighbours < 4, it becomes wall.
                if (alive_neighbors < (8 - WALL_THRESHOLD +1) ) { // +1 because count is of floor, threshold is for walls
                     next_map_state[y][x] = T_DUNGEON_WALL;
                }
            } else { // It's a wall tile
                // A wall tile becomes a floor if it has more than WALL_THRESHOLD floor neighbours
                // For typical CA, if wall neighbours > 4 (i.e. >4 floor neighbours), it becomes floor.
                // If WALL_THRESHOLD = 5, means if floor_neighbours > 5, it becomes floor
                if (alive_neighbors > WALL_THRESHOLD) {
                    next_map_state[y][x] = T_DUNGEON_FLOOR;
                }
            }
        }
    }
    map_to_smooth = next_map_state; // Apply the changes
}

// New helper: Flood fill to find connected areas
void UltimaGame::floodFill(int x, int y, const std::vector<String>& map_to_fill, std::vector<Point>& current_area_tiles, std::vector<std::vector<bool>>& visited, char floor_tile) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT || visited[y][x] || map_to_fill[y][x] != floor_tile) {
        return;
    }

    visited[y][x] = true;
    current_area_tiles.push_back(Point(x, y));

    floodFill(x + 1, y, map_to_fill, current_area_tiles, visited, floor_tile);
    floodFill(x - 1, y, map_to_fill, current_area_tiles, visited, floor_tile);
    floodFill(x, y + 1, map_to_fill, current_area_tiles, visited, floor_tile);
    floodFill(x, y - 1, map_to_fill, current_area_tiles, visited, floor_tile);
}

// New helper: Finds the largest connected area of floor_tile
void UltimaGame::findLargestConnectedArea(std::vector<Point>& out_largest_area_tiles, std::vector<String>& map_to_analyze, char floor_tile) {
    out_largest_area_tiles.clear();
    std::vector<std::vector<bool>> visited(MAP_HEIGHT, std::vector<bool>(MAP_WIDTH, false));
    
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (map_to_analyze[y][x] == floor_tile && !visited[y][x]) {
                std::vector<Point> current_area;
                floodFill(x, y, map_to_analyze, current_area, visited, floor_tile);
                if (current_area.size() > out_largest_area_tiles.size()) {
                    out_largest_area_tiles = current_area;
                }
            }
        }
    }
}

void UltimaGame::initializeDungeonMap(int from_cave_x, int from_cave_y) {
    dungeon_map.clear();
    std::vector<Point> largest_floor_area;
    bool suitable_dungeon_generated = false;

    for (int attempt = 0; attempt < MAX_DUNGEON_GENERATION_ATTEMPTS; ++attempt) {
        // 1. Initial Randomization
        std::vector<String> temp_dungeon_map;
        for (int i = 0; i < MAP_HEIGHT; ++i) {
            String row = "";
            for (int j = 0; j < MAP_WIDTH; ++j) {
                if (i == 0 || i == MAP_HEIGHT - 1 || j == 0 || j == MAP_WIDTH - 1) {
                    row += T_DUNGEON_WALL; // Outer border is always wall
                } else {
                    if ((float)rand() / RAND_MAX < INITIAL_FLOOR_CHANCE) {
                        row += T_DUNGEON_FLOOR;
                    } else {
                        row += T_DUNGEON_WALL;
                    }
                }
            }
            temp_dungeon_map.push_back(row);
        }

        // 2. Cellular Automata Iterations
        for (int i = 0; i < DUNGEON_GENERATION_ITERATIONS; ++i) {
            runCellularAutomataIteration(temp_dungeon_map);
        }

        // 3. Connectivity Analysis
        largest_floor_area.clear();
        findLargestConnectedArea(largest_floor_area, temp_dungeon_map, T_DUNGEON_FLOOR);

        if (largest_floor_area.size() >= MIN_DUNGEON_FLOOR_TILES) {
            dungeon_map = temp_dungeon_map; // Keep this generated map

            // Mark only the largest area as floor, rest as wall
            for(int y=0; y < MAP_HEIGHT; ++y) {
                for(int x=0; x < MAP_WIDTH; ++x) {
                    if (dungeon_map[y][x] == T_DUNGEON_FLOOR) {
                         bool in_largest_area = false;
                         for(const auto& p : largest_floor_area) {
                             if(p.x == x && p.y == y) {
                                 in_largest_area = true;
                                 break;
                             }
                         }
                         if (!in_largest_area) {
                             dungeon_map[y][x] = T_DUNGEON_WALL; // Fill disconnected areas
                         }
                    }
                }
            }
            // Re-run findLargestConnectedArea on the cleaned map to ensure largest_floor_area points are valid.
            findLargestConnectedArea(largest_floor_area, dungeon_map, T_DUNGEON_FLOOR);

            suitable_dungeon_generated = true;
            break; // Exit retry loop
        }
    } // End of generation attempts loop

    if (!suitable_dungeon_generated || largest_floor_area.empty()) {
        // Fallback: create a 5x5 open room if generation fails or area is too small / empty
        dungeon_map.assign(MAP_HEIGHT, String(std::string(MAP_WIDTH, T_DUNGEON_WALL).c_str())); // Fill with walls
        int center_x = MAP_WIDTH / 2;
        int center_y = MAP_HEIGHT / 2;
        int room_size = 5; // For 5x5 room
        int half_room = room_size / 2;

        largest_floor_area.clear();
        for(int r = -half_room; r <= half_room; ++r) {
            for(int c = -half_room; c <= half_room; ++c) {
                int current_y = center_y + r;
                int current_x = center_x + c;
                if(current_y > 0 && current_y < MAP_HEIGHT -1 && current_x > 0 && current_x < MAP_WIDTH -1) { // Ensure within inner map bounds
                    dungeon_map[current_y][current_x] = T_DUNGEON_FLOOR;
                    largest_floor_area.push_back(Point(current_x, current_y));
                }
            }
        }
        // If largest_floor_area is still empty (e.g. MAP_WIDTH/HEIGHT too small for 5x5), use at least one tile.
        if (largest_floor_area.empty()) {
            if (center_y > 0 && center_y < MAP_HEIGHT -1 && center_x > 0 && center_x < MAP_WIDTH -1) {
                dungeon_map[center_y][center_x] = T_DUNGEON_FLOOR;
                largest_floor_area.push_back(Point(center_x, center_y));
            } else { // Absolute fallback: top-left accessible
                 dungeon_map[1][1] = T_DUNGEON_FLOOR;
                 largest_floor_area.push_back(Point(1,1));
            }
        }
    }

    // 4. Player and Stair Placement (common for generated or fallback)
    if (largest_floor_area.empty()) { // Should be extremely rare after fallback logic
        // Absolute last resort: place player at fixed point if everything failed.
        player_x = MAP_WIDTH / 2;
        player_y = MAP_HEIGHT / 2;
        if (player_y >=0 && player_y < MAP_HEIGHT && player_x >=0 && player_x < MAP_WIDTH) {
           dungeon_map[player_y][player_x] = T_DUNGEON_FLOOR; // Ensure it's floor
        } else { // if map is tiny
           player_x = 1; player_y = 1;
           dungeon_map[1][1] = T_DUNGEON_FLOOR;
        }
        // Attempt to place stairs up somewhere, anywhere if possible
        if(player_y - 1 > 0) dungeon_map[player_y - 1][player_x] = T_STAIRS_UP;
        else if (player_x + 1 < MAP_WIDTH -1) dungeon_map[player_y][player_x+1] = T_STAIRS_UP;

    } else {
        // Place Player
        int player_start_index = rand() % largest_floor_area.size();
        Point player_start_pos = largest_floor_area[player_start_index];
        player_x = player_start_pos.x;
        player_y = player_start_pos.y;
        dungeon_map[player_y][player_x] = T_DUNGEON_FLOOR; // Ensure player starts on a clear floor tile

        // Temporarily remove player's position from consideration for stairs up
        std::vector<Point> available_for_stairs = largest_floor_area;
        available_for_stairs.erase(std::remove_if(available_for_stairs.begin(), available_for_stairs.end(),
                                           [&](const Point& p){ return p.x == player_x && p.y == player_y; }),
                           available_for_stairs.end());

        if (available_for_stairs.empty()) {
            // If only one spot was available (player took it), try to place stairs adjacent
            // This is a simplified adjacency check.
            bool placed_stairs = false;
            int offsets[] = {-1, 1};
            for(int dx_offset : offsets) { // Check horizontal first
                if (player_x + dx_offset > 0 && player_x + dx_offset < MAP_WIDTH -1 && dungeon_map[player_y][player_x + dx_offset] == T_DUNGEON_FLOOR) {
                    dungeon_map[player_y][player_x + dx_offset] = T_STAIRS_UP;
                    placed_stairs = true; break;
                }
            }
            if(!placed_stairs) {
                for(int dy_offset : offsets) { // Check vertical
                     if (player_y + dy_offset > 0 && player_y + dy_offset < MAP_HEIGHT -1 && dungeon_map[player_y + dy_offset][player_x] == T_DUNGEON_FLOOR) {
                        dungeon_map[player_y + dy_offset][player_x] = T_STAIRS_UP;
                        placed_stairs = true; break;
                    }
                }
            }
            // If still not placed, stairs might be missing or on player.
        } else {
            // Place Stairs Up
            int stairs_up_index = rand() % available_for_stairs.size();
            Point stairs_up_pos = available_for_stairs[stairs_up_index];
            dungeon_map[stairs_up_pos.y][stairs_up_pos.x] = T_STAIRS_UP;
        }
    }
}

void UltimaGame::movePlayer(int dx, int dy) {
    int new_x = player_x + dx;
    int new_y = player_y + dy;

    char target_tile;
    if (current_level == GameLevel::OVERWORLD) {
        if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT) {
            target_tile = game_map[new_y][new_x];
            if (target_tile != T_OVERWORLD_WALL) {
                player_x = new_x;
                player_y = new_y;
            }
        }
    } else { // GameLevel::DUNGEON
        if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT) {
            target_tile = dungeon_map[new_y][new_x];
            if (target_tile == T_DUNGEON_FLOOR || target_tile == T_STAIRS_UP) {
                player_x = new_x;
                player_y = new_y;
            }
        }
    }
}

String UltimaGame::renderView() {
    String view_str = "";
    int view_start_x = player_x - VIEW_WIDTH / 2;
    int view_start_y = player_y - VIEW_HEIGHT / 2;

    // --- Start of Debug Output ---
    Serial.println("--- renderView() Debug ---");
    Serial.printf("Player (X,Y): (%d,%d)\n", player_x, player_y);
    Serial.printf("VIEW_WIDTH: %d, VIEW_HEIGHT: %d\n", VIEW_WIDTH, VIEW_HEIGHT);
    Serial.printf("view_start_x: %d, view_start_y: %d\n", view_start_x, view_start_y);
    // --- End of Debug Output ---

    const std::vector<String>* current_map_ptr = (current_level == GameLevel::OVERWORLD) ? &game_map : &dungeon_map;

    for (int y_offset = 0; y_offset < VIEW_HEIGHT; ++y_offset) {
        for (int x_offset = 0; x_offset < VIEW_WIDTH; ++x_offset) {
            int map_render_x = view_start_x + x_offset;
            int map_render_y = view_start_y + y_offset;

            if (map_render_x == player_x && map_render_y == player_y) {
                view_str += T_PLAYER;
            } else if (map_render_x >= 0 && map_render_x < MAP_WIDTH &&
                       map_render_y >= 0 && map_render_y < MAP_HEIGHT) {
                char tile = (*current_map_ptr)[map_render_y][map_render_x];
                view_str += tile;
            } else {
                view_str += ' '; // Out of map bounds, show empty space
            }
        }
        if (y_offset < VIEW_HEIGHT - 1) {
             view_str += "\n";
        }
    }

    // --- Start of Debug Output ---
    Serial.println("Generated view_str:");
    // Print char by char to see exact structure, including newlines
    for (int i = 0; i < view_str.length(); i++) {
        if (view_str[i] == '\n') {
            Serial.print("[NL]" ); // Clearly mark newlines
        } else {
            Serial.print(view_str[i]);
        }
    }
    Serial.println("\n--- End renderView() Debug ---");
    // --- End of Debug Output ---
    return view_str;
}

String UltimaGame::searchCurrentTile() {
    char tile_char;
    if (current_level == GameLevel::OVERWORLD) {
        tile_char = game_map[player_y][player_x];
        switch (tile_char) {
            case T_CAVE_ENTRANCE:
                overworld_player_x_return = player_x;
                overworld_player_y_return = player_y;
                current_level = GameLevel::DUNGEON;
                initializeDungeonMap(player_x, player_y); 
                return "You enter the dark cave.";
            case T_OASIS:
                hp = max_hp; // Heal to full
                game_map[player_y][player_x] = T_SAND; // Oasis turns to sand
                return "You rest at the oasis. HP restored! The oasis dries up.";
            case T_SAND:
                return "Desert sands stretch out.";
            case T_OVERWORLD_WALL: 
                return "A rocky outcrop blocks the way.";
            default:
                return "An unknown feature.";
        }
    } else { // GameLevel::DUNGEON
        tile_char = dungeon_map[player_y][player_x];
        switch (tile_char) {
            case T_STAIRS_UP:
                current_level = GameLevel::OVERWORLD;
                player_x = overworld_player_x_return;
                player_y = overworld_player_y_return;
                // dungeon_map.clear(); // Optional: clear dungeon map to save memory if not returning to same one
                return "You climb the stairs back to the desert.";
            case T_DUNGEON_FLOOR:
                return "The air is damp and cool.";
            case T_DUNGEON_WALL:
                return "A cold, damp wall.";
            default:
                return "An odd fixture in the dungeon.";
        }
    }
}

String UltimaGame::getFormattedStats() {
    String stats_str = "LVL: " + String(level) + "\n";
    stats_str += "HP:  " + String(hp) + "/" + String(max_hp) + "\n";
    stats_str += "XP:  " + String(xp);
    return stats_str;
}

void UltimaGame::restartGame() {
    player_x = MAP_WIDTH / 2;
    player_y = MAP_HEIGHT / 2;
    current_level = GameLevel::OVERWORLD;
    // No need to re-seed random, use initial seed from constructor for consistency if desired
    initializeOverworldMap(); // Regenerates map and places player on sand
    initializeStats();        // Resets HP, level, XP
} 