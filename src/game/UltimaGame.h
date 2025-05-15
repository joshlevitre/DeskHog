#pragma once

#include <Arduino.h>
#include <vector>

#define MAP_WIDTH 20
#define MAP_HEIGHT 15
#define VIEW_WIDTH 24
#define VIEW_HEIGHT 12

class UltimaGame {
public:
    UltimaGame();

    void movePlayer(int dx, int dy);
    String renderView(); // Renders a portion of the map around the player

    int getPlayerX() const { return player_x; }
    int getPlayerY() const { return player_y; }

    String searchCurrentTile(); // New method to search the player's current tile

private:
    std::vector<String> game_map;
    int player_x;
    int player_y;

    void initializeMap();
}; 