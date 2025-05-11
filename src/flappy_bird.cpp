#include "flappy_bird.h"
#include "Arduino.h" // For random, etc.

FlappyBirdGame::FlappyBirdGame()
    : main_container(nullptr), bird_obj(nullptr), 
      bird_y(FB_SCREEN_HEIGHT / 2), bird_velocity(0.0f), game_over(false) {}

FlappyBirdGame::~FlappyBirdGame() {
    cleanup();
}

void FlappyBirdGame::setup(lv_obj_t* parent_screen) {
    main_container = lv_obj_create(parent_screen);
    lv_obj_remove_style_all(main_container);
    lv_obj_set_size(main_container, FB_SCREEN_WIDTH, FB_SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(main_container, lv_color_hex(0x4A90E2), LV_PART_MAIN); // Blue sky
    lv_obj_clear_flag(main_container, LV_OBJ_FLAG_SCROLLABLE);

    bird_obj = lv_obj_create(main_container);
    lv_obj_set_size(bird_obj, BIRD_SIZE, BIRD_SIZE);
    lv_obj_set_style_bg_color(bird_obj, lv_color_hex(0xFFD700), LV_PART_MAIN); // Yellow bird
    lv_obj_set_style_radius(bird_obj, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align(bird_obj, LV_ALIGN_LEFT_MID, 30, 0);
    lv_obj_set_y(bird_obj, bird_y - BIRD_SIZE / 2);

    // TODO: Initialize pipes, score label, etc.
    bird_y = FB_SCREEN_HEIGHT / 2;
    bird_velocity = 0.0f;
    game_over = false;
}

void FlappyBirdGame::loop() {
    if (game_over) {
        // Handle game over input (e.g., press center to restart)
        if (Input::isCenterPressed()) { // Assuming Input::isCenterPressed() exists and is updated
            setup(lv_obj_get_parent(main_container)); // Re-setup on the same parent
        }
        return;
    }

    handle_input();
    update_game_state();
    render();
}

void FlappyBirdGame::handle_input() {
    // Example: Flap on UP button press
    if (Input::isUpPressed()) { // Assuming Input::isUpPressed() exists and is updated
        bird_velocity = -4.0f; // Flap strength (adjust as needed)
    }
}

void FlappyBirdGame::update_game_state() {
    // Apply gravity
    bird_velocity += 0.2f; // Gravity strength (adjust as needed)
    bird_y += (int)bird_velocity;

    // Check boundaries (simple version)
    if (bird_y + BIRD_SIZE / 2 > FB_SCREEN_HEIGHT || bird_y - BIRD_SIZE / 2 < 0) {
        game_over = true;
        // Could add a sound or visual cue for game over here
    }
    // TODO: Add pipe generation, movement, collision detection with pipes
}

void FlappyBirdGame::render() {
    if (bird_obj) {
        lv_obj_set_y(bird_obj, bird_y - BIRD_SIZE / 2);
    }
    // TODO: Render pipes, score
}

void FlappyBirdGame::cleanup() {
    if (main_container) {
        lv_obj_del(main_container); // This will delete all children too
        main_container = nullptr;
        bird_obj = nullptr; // It was a child of main_container
    }
}

lv_obj_t* FlappyBirdGame::get_main_container() {
    return main_container;
} 