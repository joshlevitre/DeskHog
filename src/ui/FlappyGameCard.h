#pragma once

#include <lvgl.h>
#include <Arduino.h>
#include <vector>
#include "ui/InputHandler.h"

/**
 * @class FlappyGameCard
 * @brief A Flappy Bird-style game using simple rectangles and circles
 * 
 * Features:
 * - Player character (circle) that jumps when center button is pressed
 * - Obstacles (rectangles) that move from right to left
 * - Score tracking
 * - Game over state with restart capability
 * - Minimal graphics using only rectangles and circles
 */
class FlappyGameCard : public InputHandler {
public:
    /**
     * @brief Constructor
     * @param parent LVGL parent object
     * 
     * Creates a full-width/height game card
     */
    FlappyGameCard(lv_obj_t* parent);
    
    /**
     * @brief Destructor - safely cleans up UI resources
     */
    ~FlappyGameCard();
    
    /**
     * @brief Get the underlying LVGL card object
     * @return LVGL object pointer or nullptr if not created
     */
    lv_obj_t* getCard();
    
    /**
     * @brief Start the game
     */
    void startGame();
    
    /**
     * @brief Pause the game
     */
    void pauseGame();
    
    /**
     * @brief Resume the game
     */
    void resumeGame();
    
    /**
     * @brief Reset the game to initial state
     */
    void resetGame();
    
    /**
     * @brief Handle button press events
     * @param button_index The index of the button that was pressed
     * @return true if center button (jump), false otherwise
     */
    bool handleButtonPress(uint8_t button_index) override;
    
    /**
     * @brief Update game state (called from game loop)
     */
    void update();
    
private:
    // Game constants
    static constexpr int PLAYER_SIZE = 8;           ///< Player circle radius
    static constexpr int OBSTACLE_WIDTH = 20;       ///< Obstacle width
    static constexpr int OBSTACLE_GAP = 40;         ///< Gap between top and bottom obstacles
    static constexpr int OBSTACLE_SPEED = 2;        ///< Pixels per frame obstacle speed
    static constexpr int GRAVITY = 1;               ///< Gravity acceleration
    static constexpr int JUMP_VELOCITY = -8;        ///< Initial jump velocity
    static constexpr int OBSTACLE_SPAWN_X = 240;    ///< X position where obstacles spawn
    static constexpr int OBSTACLE_SPAWN_INTERVAL = 120; ///< Frames between obstacle spawns
    
    // Game state
    enum class GameState {
        MENU,       ///< Main menu state
        PLAYING,    ///< Game is active
        PAUSED,     ///< Game is paused
        GAME_OVER   ///< Game over state
    };
    
    // Player structure
    struct Player {
        int x = 50;                 ///< X position
        int y = 67;                 ///< Y position (center of screen)
        int velocity = 0;           ///< Vertical velocity
        bool alive = true;          ///< Player alive status
    };
    
    // Obstacle structure
    struct Obstacle {
        int x;                      ///< X position
        int topHeight;              ///< Height of top obstacle
        int bottomY;                ///< Y position of bottom obstacle
        bool passed = false;        ///< Whether player has passed this obstacle
    };
    
    // UI Elements
    lv_obj_t* _parent;             ///< LVGL parent object
    lv_obj_t* _card;               ///< Main card container
    lv_obj_t* _game_area;          ///< Game drawing area
    lv_obj_t* _player_obj;         ///< Player circle object
    std::vector<lv_obj_t*> _obstacles; ///< Obstacle rectangle objects
    lv_obj_t* _score_label;        ///< Score display label
    lv_obj_t* _game_over_label;    ///< Game over message label
    lv_obj_t* _menu_label;         ///< Menu instructions label
    
    // Game data
    GameState _game_state;         ///< Current game state
    Player _player;                ///< Player data
    std::vector<Obstacle> _obstacles_data; ///< Obstacle data
    int _score;                    ///< Current score
    int _frame_count;              ///< Frame counter for timing
    int _last_obstacle_spawn;      ///< Last frame when obstacle was spawned
    
    /**
     * @brief Create the main card UI
     */
    void createCard();
    
    /**
     * @brief Create the game area
     */
    void createGameArea();
    
    /**
     * @brief Create the player object
     */
    void createPlayer();
    
    /**
     * @brief Create UI labels
     */
    void createLabels();
    
    /**
     * @brief Update player physics
     */
    void updatePlayer();
    
    /**
     * @brief Update obstacles
     */
    void updateObstacles();
    
    /**
     * @brief Spawn a new obstacle
     */
    void spawnObstacle();
    
    /**
     * @brief Check for collisions
     */
    void checkCollisions();
    
    /**
     * @brief Update score
     */
    void updateScore();
    
    /**
     * @brief Update UI elements
     */
    void updateUI();
    
    /**
     * @brief Show game over screen
     */
    void showGameOver();
    
    /**
     * @brief Show menu screen
     */
    void showMenu();
    
    /**
     * @brief Hide all game elements
     */
    void hideGameElements();
    
    /**
     * @brief Show game elements
     */
    void showGameElements();
    
    /**
     * @brief Check if an LVGL object is valid
     * @param obj LVGL object to check
     * @return true if object exists and is valid
     */
    bool isValidObject(lv_obj_t* obj) const;
}; 