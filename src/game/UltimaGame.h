#pragma once

#include <Arduino.h>
#include <vector>

#define MAP_WIDTH 20
#define MAP_HEIGHT 15
#define VIEW_WIDTH 9  // Adjusted for unscii_16 font (145px / 8px_char = ~18, using 17 for odd)
#define VIEW_HEIGHT 5 // Adjusted for unscii_16 font (111px / 16px_char = ~6, using 5 for odd)

// Tile Definitions (Chars)
const char T_SAND = '.';
const char T_OASIS = '~';
const char T_CAVE_ENTRANCE = 'O'; // Overworld representation of a cave
const char T_DUNGEON_WALL = '#';
const char T_DUNGEON_FLOOR = ' ';
const char T_STAIRS_UP = '<';   // Leads from dungeon to overworld
// const char T_STAIRS_DOWN = '>'; // Leads from overworld to dungeon (player appears on this in dungeon) - REMOVED
const char T_OVERWORLD_WALL = '#'; // Existing wall type, now specific to overworld boundary

// Custom Game Tile Definitions (UTF-8 Strings)
const char T_PLAYER[] = "\xEE\xB0\xA8"; // Person symbol

// Game Level Management
enum class GameLevel {
    OVERWORLD,
    DUNGEON
};

class UltimaGame {
public:
    UltimaGame();

    void movePlayer(int dx, int dy);
    String renderView(); // Renders a portion of the map around the player

    int getPlayerX() const { return player_x; }
    int getPlayerY() const { return player_y; }

    // Player Stats Getters
    int getHP() const { return hp; }
    int getMaxHP() const { return max_hp; }
    int getLevel() const { return level; }
    int getXP() const { return xp; }

    String searchCurrentTile(); // New method to search the player's current tile
    String getFormattedStats(); // Method to get stats as a displayable string
    void restartGame(); // New: Method to restart the game

private:
    std::vector<String> game_map; // Represents the overworld
    std::vector<String> dungeon_map; // Represents the current dungeon level
    GameLevel current_level;

    int player_x;
    int player_y;
    int overworld_player_x_return; // To store player's X on overworld when entering dungeon
    int overworld_player_y_return; // To store player's Y on overworld when entering dungeon

    // Player Stats
    int hp;
    int max_hp;
    int level;
    int xp;

    void initializeOverworldMap(); // Renamed from initializeMap
    void initializeDungeonMap(int from_cave_x, int from_cave_y); // For generating a new dungeon
    void initializeStats();

    // Constants for Cellular Automata
    static const int DUNGEON_GENERATION_ITERATIONS = 5; // How many times to run the CA simulation
    static const int WALL_THRESHOLD = 5; // If a cell has this many or more wall neighbours, it becomes a wall
    static const float INITIAL_FLOOR_CHANCE; // 45% chance for a tile to be initially floor
    static const int MIN_DUNGEON_FLOOR_TILES = 25; // Minimum number of floor tiles for a valid dungeon
    static const int MAX_DUNGEON_GENERATION_ATTEMPTS = 5; // Max attempts to generate a suitable dungeon

    struct Point { 
        int x, y;
        // Constructor for easy initialization
        Point(int _x = 0, int _y = 0) : x(_x), y(_y) {}
        // Equality operator for comparing points, useful for std::vector::erase or std::find
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };

    // Helper methods for dungeon generation
    void runCellularAutomataIteration(std::vector<String>& map_to_smooth);
    int countAliveNeighbors(const std::vector<String>& map_to_check, int x, int y, char wall_tile, char floor_tile);
    void findLargestConnectedArea(std::vector<Point>& out_largest_area_tiles, std::vector<String>& map_to_analyze, char floor_tile);
    void floodFill(int x, int y, const std::vector<String>& map_to_fill, std::vector<Point>& current_area_tiles, std::vector<std::vector<bool>>& visited, char floor_tile);
}; 