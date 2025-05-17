#include "UltimaGame.h"
#include <cstdlib> // For rand()
#include <ctime>   // For time() for seeding rand()
#include <string>  // For std::string

// Definition for the static const float member
const float UltimaGame::INITIAL_FLOOR_CHANCE = 0.45f;

UltimaGame::UltimaGame() : player_x(MAP_WIDTH / 2), player_y(MAP_HEIGHT / 2), current_level(GameLevel::OVERWORLD), player_defeated_flag(false) {
    std::srand(std::time(0)); // Seed random number generator
    initializeOverworldMap();
    initializeStats(); // Initialize player stats
    turn_message = ""; // Initialize turn message
}

void UltimaGame::initializeStats() {
    hp = 15;
    max_hp = 15;
    level = 1;
    xp = 0;
    player_attack = PLAYER_ATTACK_DAMAGE; 
    player_defeated_flag = false;
    player_moves_count = 0; // Initialize player moves count
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
    monsters.clear(); // Clear monsters for new dungeon
    player_defeated_flag = false; // Reset defeat flag
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
    Point stairs_up_pos(-1,-1); // To ensure monsters don't spawn on stairs
    if (largest_floor_area.empty()) {
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
        dungeon_map[player_y][player_x] = T_DUNGEON_FLOOR; 

        std::vector<Point> available_for_spawn = largest_floor_area;
        available_for_spawn.erase(std::remove_if(available_for_spawn.begin(), available_for_spawn.end(),
                                           [&](const Point& p){ return p.x == player_x && p.y == player_y; }),
                           available_for_spawn.end());

        if (available_for_spawn.empty()) {
            // Only one spot, player took it. Try adjacent for stairs.
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
            // For simplicity, assume stairs_up_pos is set if successful.
            // Example: if(placed_stairs) stairs_up_pos = Point(target_x, target_y);
        } else {
            int stairs_up_index = rand() % available_for_spawn.size();
            stairs_up_pos = available_for_spawn[stairs_up_index];
            dungeon_map[stairs_up_pos.y][stairs_up_pos.x] = T_STAIRS_UP;
            
            // Remove stairs pos from spawn consideration
            available_for_spawn.erase(std::remove_if(available_for_spawn.begin(), available_for_spawn.end(),
                                           [&](const Point& p){ return p.x == stairs_up_pos.x && p.y == stairs_up_pos.y; }),
                               available_for_spawn.end());
        }
    }

    // 5. Monster Spawning
    if (!largest_floor_area.empty()) { 
        std::vector<Point> spawn_points = largest_floor_area;
        spawn_points.erase(std::remove_if(spawn_points.begin(), spawn_points.end(),
                                    [&](const Point& p){ return (p.x == player_x && p.y == player_y) || 
                                                              (stairs_up_pos.x != -1 && p.x == stairs_up_pos.x && p.y == stairs_up_pos.y); }),
                            spawn_points.end());

        int max_spawnable_here = MAX_MONSTERS_PER_DUNGEON;
        if (spawn_points.size() < max_spawnable_here) {
            max_spawnable_here = spawn_points.size();
        }

        int num_monsters_to_spawn = 0;
        if (max_spawnable_here > 0) {
            num_monsters_to_spawn = rand() % (max_spawnable_here + 1); // Random 0 to max_spawnable_here
        }
        
        for (int i = 0; i < num_monsters_to_spawn; ++i) {
            if (spawn_points.empty()) break; 
            int monster_spawn_idx = rand() % spawn_points.size();
            Point monster_pos = spawn_points[monster_spawn_idx];
            int monster_hp = (rand() % 9) + 1; 
            monsters.emplace_back(monster_pos.x, monster_pos.y, monster_hp, MONSTER_ATTACK_DAMAGE);
            spawn_points.erase(spawn_points.begin() + monster_spawn_idx); 
        }
    }
}

void UltimaGame::movePlayer(int dx, int dy) {
    if (player_defeated_flag) return; // Can't move if defeated

    int new_x = player_x + dx;
    int new_y = player_y + dy;

    clearTurnMessage(); // Clear previous turn messages before player action

    if (current_level == GameLevel::OVERWORLD) {
        if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT) {
            char target_tile = game_map[new_y][new_x];
            if (target_tile != T_OVERWORLD_WALL) {
                player_x = new_x;
                player_y = new_y;
                player_moves_count++;
                // turn_message += "Moved. "; // Basic move message if needed, or handled by search
            }
        }
    } else { // GameLevel::DUNGEON
        if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT) {
            // Check for monster at target location first
            Monster* target_monster = nullptr;
            for (auto& monster : monsters) {
                if (monster.active && monster.x == new_x && monster.y == new_y) {
                    target_monster = &monster;
                    break;
                }
            }

            if (target_monster) {
                resolveCombat(*target_monster); // Combat happens, turn_message is populated by resolveCombat
                // Player does not move into monster's square unless monster is defeated AND we then allow it.
                // For now, combat resolution itself is the action.
                // If monster was defeated, it's now inactive. Player could move in a subsequent turn or if we add post-combat move here.
            } else {
                char target_tile = dungeon_map[new_y][new_x];
                if (target_tile == T_DUNGEON_FLOOR || target_tile == T_STAIRS_UP) { 
                    player_x = new_x;
                    player_y = new_y;
                    player_moves_count++;
                    // turn_message += "Moved. ";
                } else if (target_tile == T_DUNGEON_WALL) {
                    turn_message += "Blocked by a wall. ";
                } else {
                    // Potentially other non-passable dungeon features in future
                    turn_message += "Cannot move there. ";
                }
            }
        }
    }
}

String UltimaGame::renderView() {
    String view_str = "";
    int view_start_x = player_x - VIEW_WIDTH / 2;
    int view_start_y = player_y - VIEW_HEIGHT / 2;

    const std::vector<String>* current_map_ptr = (current_level == GameLevel::OVERWORLD) ? &game_map : &dungeon_map;

    for (int y_offset = 0; y_offset < VIEW_HEIGHT; ++y_offset) {
        for (int x_offset = 0; x_offset < VIEW_WIDTH; ++x_offset) {
            int map_render_x = view_start_x + x_offset;
            int map_render_y = view_start_y + y_offset;

            if (map_render_x == player_x && map_render_y == player_y) {
                view_str += T_PLAYER;
            } else {
                bool monster_rendered = false;
                if (current_level == GameLevel::DUNGEON) {
                    for (const auto& monster : monsters) {
                        if (monster.active && monster.x == map_render_x && monster.y == map_render_y) {
                            view_str += T_MONSTER;
                            monster_rendered = true;
                            break;
                        }
                    }
                }
                if (!monster_rendered) {
                    if (map_render_x >= 0 && map_render_x < MAP_WIDTH &&
                        map_render_y >= 0 && map_render_y < MAP_HEIGHT) {
                        char tile = (*current_map_ptr)[map_render_y][map_render_x];
                        view_str += tile;
                    } else {
                        view_str += ' '; // Out of map bounds
                    }
                }
            }
        }
        if (y_offset < VIEW_HEIGHT - 1) {
             view_str += "\n";
        }
    }
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
    initializeOverworldMap(); 
    initializeStats(); // This will also reset player_moves_count to 0 via initializeStats call      
    monsters.clear(); 
    clearTurnMessage();
    player_defeated_flag = false;
    // player_moves_count = 0; // No longer needed here as initializeStats handles it
}

// New message handling methods
void UltimaGame::clearTurnMessage() {
    turn_message = "";
}

String UltimaGame::getTurnMessageAndClear() {
    String temp_msg = turn_message;
    turn_message = "";
    return temp_msg;
}

String UltimaGame::resolveCombat(Monster& monster) {
    clearTurnMessage();

    if (player_defeated_flag || !monster.active) {
        return ""; 
    }

    // 1. Player attacks monster
    float player_hit_roll = (float)rand() / RAND_MAX;
    float player_current_hit_chance = PLAYER_BASE_HIT_CHANCE + ((xp / 10) * PLAYER_HIT_CHANCE_PER_10_XP_INCREMENT);
    player_current_hit_chance = constrain(player_current_hit_chance, 0.0f, 1.0f); // Cap at 100%

    if (player_hit_roll <= player_current_hit_chance) {
        monster.hp -= PLAYER_ATTACK_DAMAGE;
        turn_message += "You hit Monster for " + String(PLAYER_ATTACK_DAMAGE) + " dmg. ";
    } else {
        turn_message += "You missed Monster. ";
    }

    if (monster.hp <= 0) {
        monster.active = false;
        // hp = constrain(hp, 0, max_hp); // Player HP is not affected here directly
        xp += MONSTER_XP_REWARD;
        turn_message += "Monster defeated! You gain " + String(MONSTER_XP_REWARD) + " XP.";
        // TODO: Check for level up here
        return turn_message; 
    }

    // 2. Monster attacks player (if still active)
    float monster_hit_roll = (float)rand() / RAND_MAX;
    float monster_current_hit_chance = MONSTER_BASE_HIT_CHANCE + (player_moves_count * MONSTER_HIT_CHANCE_PER_PLAYER_MOVE_INCREMENT);
    monster_current_hit_chance = constrain(monster_current_hit_chance, 0.0f, 1.0f); // Cap at 100%

    if (monster_hit_roll <= monster_current_hit_chance) {
        hp -= MONSTER_ATTACK_DAMAGE;
        turn_message += "Monster hits you for " + String(MONSTER_ATTACK_DAMAGE) + " dmg.";
    } else {
        turn_message += "Monster missed you.";
    }

    if (hp <= 0) {
        hp = 0;
        player_defeated_flag = true;
        turn_message += " You have been defeated!";
    }
    
    return turn_message; 
}

void UltimaGame::moveMonsters() {
    if (current_level != GameLevel::DUNGEON || player_defeated_flag) {
        return; // Monsters only move in dungeons and if player is active
    }

    // Using a temporary string for monster actions to append to main turn_message later
    // This helps keep player action messages separate from monster action messages if needed.
    String monster_actions_msg = ""; 

    for (auto& monster : monsters) {
        if (!monster.active) continue;

        int sight_range = (rand() % 7) + 6; // Random sight range 6-12 tiles

        // Check if player is within sight_range (simple Manhattan distance)
        int dist_to_player_x = abs(player_x - monster.x);
        int dist_to_player_y = abs(player_y - monster.y);

        if (dist_to_player_x <= sight_range && dist_to_player_y <= sight_range) { // Player is visible
            int dx = 0, dy = 0;
            if (player_x < monster.x) dx = -1;
            else if (player_x > monster.x) dx = 1;

            if (player_y < monster.y) dy = -1;
            else if (player_y > monster.y) dy = 1;

            // Try to move on X-axis first, then Y if X is blocked or not needed
            int next_monster_x = monster.x + dx;
            int next_monster_y = monster.y;
            bool moved_on_x = false;

            if (dx != 0) { // If there is an X-component to move
                if (next_monster_x == player_x && next_monster_y == player_y) {
                    monster_actions_msg += resolveCombat(monster); // Monster attacks player
                    if (player_defeated_flag) break; // Stop other monsters if player is defeated
                    continue; // Next monster
                } else if (dungeon_map[next_monster_y][next_monster_x] == T_DUNGEON_FLOOR) {
                    bool occupied_by_other_monster = false;
                    for (const auto& other_monster : monsters) {
                        if (&other_monster != &monster && other_monster.active && 
                            other_monster.x == next_monster_x && other_monster.y == next_monster_y) {
                            occupied_by_other_monster = true;
                            break;
                        }
                    }
                    if (!occupied_by_other_monster) {
                        monster.x = next_monster_x;
                        // monster_actions_msg += "Monster moves. "; // Optional: message for simple move
                        moved_on_x = true;
                    }
                }
            }

            if (!moved_on_x && dy != 0) { // If no X-move or X-move wasn't taken, try Y-move
                next_monster_x = monster.x; // Reset X for Y-only move consideration
                next_monster_y = monster.y + dy;
                if (next_monster_x == player_x && next_monster_y == player_y) {
                    monster_actions_msg += resolveCombat(monster); // Monster attacks player
                    if (player_defeated_flag) break; 
                    continue; // Next monster
                } else if (dungeon_map[next_monster_y][next_monster_x] == T_DUNGEON_FLOOR) {
                    bool occupied_by_other_monster = false;
                    for (const auto& other_monster : monsters) {
                        if (&other_monster != &monster && other_monster.active && 
                            other_monster.x == next_monster_x && other_monster.y == next_monster_y) {
                            occupied_by_other_monster = true;
                            break;
                        }
                    }
                    if (!occupied_by_other_monster) {
                        monster.y = next_monster_y;
                        // monster_actions_msg += "Monster moves. ";
                    }
                }
            }
        } // end if player visible
        if (player_defeated_flag) break; // Stop processing other monsters if player got defeated by one
    } // end for each monster

    if (!monster_actions_msg.isEmpty()) {
        if (!turn_message.isEmpty() && !turn_message.endsWith(" ")) {
            turn_message += " "; // Add a space if there was a previous player message
        }
        turn_message += monster_actions_msg; // Append monster actions to overall turn message
    }
} 