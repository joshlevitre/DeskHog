#ifndef FLAPPY_BIRD_H
#define FLAPPY_BIRD_H

#include "lvgl.h"
#include "hardware/Input.h" // For button inputs

// Basic game constants (can be expanded)
const int FB_SCREEN_WIDTH = 240;
const int FB_SCREEN_HEIGHT = 135;
const int BIRD_SIZE = 10;

class FlappyBirdGame {
public:
    FlappyBirdGame();
    ~FlappyBirdGame();

    void setup(lv_obj_t* parent_screen); // Creates the game's UI elements on parent_screen
    void loop();                     // Main game logic tick, called when this card is active
    void cleanup();                  // Cleans up LVGL objects
    lv_obj_t* get_main_container();   // Returns the root LVGL object for this game/card

private:
    void handle_input();
    void update_game_state();
    void render();

    lv_obj_t* main_container; // The root object for this game, to be added to CardNavigationStack
    lv_obj_t* bird_obj;
    // Add other game elements like pipes, score label, etc.

    // Game state variables
    int bird_y;
    float bird_velocity;
    bool game_over;
};

#endif // FLAPPY_BIRD_H 