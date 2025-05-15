#pragma once

#include <Arduino.h>
#include <vector>

#define MAP_WIDTH 20
#define MAP_HEIGHT 15
#define VIEW_WIDTH 16  // Adjusted for side stats panel
#define VIEW_HEIGHT 7  // Adjusted for bottom message log

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

private:
    std::vector<String> game_map;
    int player_x;
    int player_y;

    // Player Stats
    int hp;
    int max_hp;
    int level;
    int xp;

    void initializeMap();
    void initializeStats(); // New method to set up initial stats
}; 