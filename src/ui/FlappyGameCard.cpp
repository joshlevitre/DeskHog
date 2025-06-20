#include "ui/FlappyGameCard.h"
#include "Style.h"
#include "hardware/Input.h"

FlappyGameCard::FlappyGameCard(lv_obj_t* parent)
    : _parent(parent)
    , _card(nullptr)
    , _game_area(nullptr)
    , _player_obj(nullptr)
    , _score_label(nullptr)
    , _game_over_label(nullptr)
    , _menu_label(nullptr)
    , _game_state(GameState::MENU)
    , _score(0)
    , _frame_count(0)
    , _last_obstacle_spawn(0) {
    
    Serial.println("[FlappyGame] Constructor called");
    createCard();
    Serial.println("[FlappyGame] Constructor completed");
}

FlappyGameCard::~FlappyGameCard() {
    // Clean up UI elements safely
    if (isValidObject(_card)) {
        lv_obj_add_flag(_card, LV_OBJ_FLAG_HIDDEN);
        lv_obj_del_async(_card);
        
        // When card is deleted, all children are automatically deleted
        _card = nullptr;
        _game_area = nullptr;
        _player_obj = nullptr;
        _score_label = nullptr;
        _game_over_label = nullptr;
        _menu_label = nullptr;
    }
}

lv_obj_t* FlappyGameCard::getCard() {
    return _card;
}

void FlappyGameCard::createCard() {
    Serial.println("[FlappyGame] Creating card...");
    
    // Create main card with black background - use parent instead of nullptr
    _card = lv_obj_create(_parent);
    if (!_card) {
        Serial.println("[FlappyGame-ERROR] Failed to create main card object");
        return;
    }
    
    Serial.println("[FlappyGame] Main card created successfully");
    
    // Set card size and style - black background
    lv_obj_set_width(_card, lv_pct(100));
    lv_obj_set_height(_card, lv_pct(100));
    lv_obj_set_style_bg_color(_card, lv_color_black(), 0);
    lv_obj_set_style_border_width(_card, 0, 0);
    lv_obj_set_style_pad_all(_card, 0, 0);
    lv_obj_set_style_margin_all(_card, 0, 0);
    
    createGameArea();
    createPlayer();
    createLabels();
    
    // Start in menu state
    showMenu();
    
    Serial.println("[FlappyGame] Card creation completed");
}

void FlappyGameCard::createGameArea() {
    _game_area = lv_obj_create(_card);
    if (!_game_area) return;
    
    // Make game area fill the entire card
    lv_obj_set_width(_game_area, lv_pct(100));
    lv_obj_set_height(_game_area, lv_pct(100));
    lv_obj_set_style_bg_color(_game_area, lv_color_hex(0x87CEEB), 0); // Sky blue background
    lv_obj_set_style_border_width(_game_area, 0, 0);
    lv_obj_set_style_pad_all(_game_area, 0, 0);
    lv_obj_set_style_radius(_game_area, 0, 0);
}

void FlappyGameCard::createPlayer() {
    _player_obj = lv_obj_create(_game_area);
    if (!_player_obj) return;
    
    // Create player as a circle
    lv_obj_set_size(_player_obj, PLAYER_SIZE * 2, PLAYER_SIZE * 2);
    lv_obj_set_style_radius(_player_obj, PLAYER_SIZE, 0);
    lv_obj_set_style_bg_color(_player_obj, lv_color_hex(0xFFFF00), 0); // Yellow player
    lv_obj_set_style_border_width(_player_obj, 0, 0);
    lv_obj_set_style_pad_all(_player_obj, 0, 0);
    
    // Position player at starting position
    lv_obj_set_pos(_player_obj, _player.x - PLAYER_SIZE, _player.y - PLAYER_SIZE);
}

void FlappyGameCard::createLabels() {
    // Create score label
    _score_label = lv_label_create(_game_area);
    if (_score_label) {
        lv_obj_set_style_text_font(_score_label, Style::valueFont(), 0);
        lv_obj_set_style_text_color(_score_label, lv_color_white(), 0);
        lv_obj_align(_score_label, LV_ALIGN_TOP_MID, 0, 10);
        lv_label_set_text(_score_label, "0");
    }
    
    // Create game over label
    _game_over_label = lv_label_create(_game_area);
    if (_game_over_label) {
        lv_obj_set_style_text_font(_game_over_label, Style::loudNoisesFont(), 0);
        lv_obj_set_style_text_color(_game_over_label, lv_color_white(), 0);
        lv_obj_set_style_text_align(_game_over_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(_game_over_label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(_game_over_label, "GAME OVER\nPress CENTER to restart");
        lv_obj_add_flag(_game_over_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Create menu label
    _menu_label = lv_label_create(_game_area);
    if (_menu_label) {
        lv_obj_set_style_text_font(_menu_label, Style::loudNoisesFont(), 0);
        lv_obj_set_style_text_color(_menu_label, lv_color_white(), 0);
        lv_obj_set_style_text_align(_menu_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(_menu_label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(_menu_label, "FLAPPY HOG\nPress CENTER to start");
    }
}

void FlappyGameCard::startGame() {
    _game_state = GameState::PLAYING;
    _score = 0;
    _frame_count = 0;
    _last_obstacle_spawn = 0;
    
    // Reset player
    _player.x = 50;
    _player.y = 67;
    _player.velocity = 0;
    _player.alive = true;
    
    // Clear obstacles
    for (auto* obstacle : _obstacles) {
        if (isValidObject(obstacle)) {
            lv_obj_del(obstacle);
        }
    }
    _obstacles.clear();
    _obstacles_data.clear();
    
    showGameElements();
    updateUI();
}

void FlappyGameCard::pauseGame() {
    if (_game_state == GameState::PLAYING) {
        _game_state = GameState::PAUSED;
    }
}

void FlappyGameCard::resumeGame() {
    if (_game_state == GameState::PAUSED) {
        _game_state = GameState::PLAYING;
    }
}

void FlappyGameCard::resetGame() {
    startGame();
}

bool FlappyGameCard::handleButtonPress(uint8_t button_index) {
    if (button_index == Input::BUTTON_CENTER) {
        switch (_game_state) {
            case GameState::MENU:
                startGame();
                return true;
                
            case GameState::PLAYING:
                // Make player jump
                if (_player.alive) {
                    _player.velocity = JUMP_VELOCITY;
                }
                return true;
                
            case GameState::GAME_OVER:
                resetGame();
                return true;
                
            case GameState::PAUSED:
                resumeGame();
                return true;
        }
    }
    
    return false;
}

void FlappyGameCard::update() {
    if (_game_state != GameState::PLAYING) {
        return;
    }
    
    _frame_count++;
    
    updatePlayer();
    updateObstacles();
    checkCollisions();
    updateScore();
    updateUI();
    
    // Spawn obstacles
    if (_frame_count - _last_obstacle_spawn >= OBSTACLE_SPAWN_INTERVAL) {
        spawnObstacle();
        _last_obstacle_spawn = _frame_count;
    }
}

void FlappyGameCard::updatePlayer() {
    if (!_player.alive) return;
    
    // Apply gravity
    _player.velocity += GRAVITY;
    _player.y += _player.velocity;
    
    // Keep player on screen
    if (_player.y < PLAYER_SIZE) {
        _player.y = PLAYER_SIZE;
        _player.velocity = 0;
    }
    if (_player.y > 135 - PLAYER_SIZE) {
        _player.y = 135 - PLAYER_SIZE;
        _player.alive = false;
        showGameOver();
    }
    
    // Update player position
    if (isValidObject(_player_obj)) {
        lv_obj_set_pos(_player_obj, _player.x - PLAYER_SIZE, _player.y - PLAYER_SIZE);
    }
}

void FlappyGameCard::updateObstacles() {
    for (size_t i = 0; i < _obstacles_data.size(); i++) {
        auto& obstacle_data = _obstacles_data[i];
        
        // Move obstacle left
        obstacle_data.x -= OBSTACLE_SPEED;
        
        // Check if player passed this obstacle
        if (!obstacle_data.passed && obstacle_data.x + OBSTACLE_WIDTH < _player.x) {
            obstacle_data.passed = true;
            _score++;
        }
        
        // Remove obstacles that are off screen
        if (obstacle_data.x + OBSTACLE_WIDTH < 0) {
            // Remove both top and bottom obstacles for this pair
            if (i * 2 + 1 < _obstacles.size()) {
                if (isValidObject(_obstacles[i * 2])) {
                    lv_obj_del(_obstacles[i * 2]);
                }
                if (isValidObject(_obstacles[i * 2 + 1])) {
                    lv_obj_del(_obstacles[i * 2 + 1]);
                }
                _obstacles.erase(_obstacles.begin() + i * 2, _obstacles.begin() + i * 2 + 2);
            }
            _obstacles_data.erase(_obstacles_data.begin() + i);
            i--; // Adjust index after removal
            continue;
        }
        
        // Update obstacle positions
        if (i * 2 + 1 < _obstacles.size()) {
            if (isValidObject(_obstacles[i * 2])) {
                lv_obj_set_pos(_obstacles[i * 2], obstacle_data.x, 0);
            }
            if (isValidObject(_obstacles[i * 2 + 1])) {
                lv_obj_set_pos(_obstacles[i * 2 + 1], obstacle_data.x, obstacle_data.bottomY);
            }
        }
    }
}

void FlappyGameCard::spawnObstacle() {
    // Create top obstacle
    lv_obj_t* top_obstacle = lv_obj_create(_game_area);
    if (!top_obstacle) return;
    
    int top_height = random(20, 80); // Random height between 20-80 pixels
    
    lv_obj_set_size(top_obstacle, OBSTACLE_WIDTH, top_height);
    lv_obj_set_style_bg_color(top_obstacle, lv_color_hex(0x228B22), 0); // Forest green
    lv_obj_set_style_border_width(top_obstacle, 0, 0);
    lv_obj_set_style_radius(top_obstacle, 0, 0);
    lv_obj_set_pos(top_obstacle, OBSTACLE_SPAWN_X, 0);
    
    // Create bottom obstacle
    lv_obj_t* bottom_obstacle = lv_obj_create(_game_area);
    if (!bottom_obstacle) {
        lv_obj_del(top_obstacle);
        return;
    }
    
    int bottom_height = 135 - top_height - OBSTACLE_GAP;
    int bottom_y = top_height + OBSTACLE_GAP;
    
    lv_obj_set_size(bottom_obstacle, OBSTACLE_WIDTH, bottom_height);
    lv_obj_set_style_bg_color(bottom_obstacle, lv_color_hex(0x228B22), 0); // Forest green
    lv_obj_set_style_border_width(bottom_obstacle, 0, 0);
    lv_obj_set_style_radius(bottom_obstacle, 0, 0);
    lv_obj_set_pos(bottom_obstacle, OBSTACLE_SPAWN_X, bottom_y);
    
    // Store obstacle data
    Obstacle obstacle_data;
    obstacle_data.x = OBSTACLE_SPAWN_X;
    obstacle_data.topHeight = top_height;
    obstacle_data.bottomY = bottom_y;
    
    _obstacles.push_back(top_obstacle);
    _obstacles.push_back(bottom_obstacle);
    _obstacles_data.push_back(obstacle_data);
}

void FlappyGameCard::checkCollisions() {
    if (!_player.alive) return;
    
    // Check collision with obstacles
    for (size_t i = 0; i < _obstacles_data.size(); i++) {
        const auto& obstacle_data = _obstacles_data[i];
        
        // Check if player is within obstacle's x range
        if (_player.x + PLAYER_SIZE > obstacle_data.x && 
            _player.x - PLAYER_SIZE < obstacle_data.x + OBSTACLE_WIDTH) {
            
            // Check collision with top obstacle
            if (_player.y - PLAYER_SIZE < obstacle_data.topHeight) {
                _player.alive = false;
                showGameOver();
                return;
            }
            
            // Check collision with bottom obstacle
            if (_player.y + PLAYER_SIZE > obstacle_data.bottomY) {
                _player.alive = false;
                showGameOver();
                return;
            }
        }
    }
}

void FlappyGameCard::updateScore() {
    if (isValidObject(_score_label)) {
        char score_text[16];
        snprintf(score_text, sizeof(score_text), "%d", _score);
        lv_label_set_text(_score_label, score_text);
    }
}

void FlappyGameCard::updateUI() {
    // Update score label position to stay visible
    if (isValidObject(_score_label)) {
        lv_obj_align(_score_label, LV_ALIGN_TOP_MID, 0, 10);
    }
}

void FlappyGameCard::showGameOver() {
    _game_state = GameState::GAME_OVER;
    
    if (isValidObject(_game_over_label)) {
        lv_obj_clear_flag(_game_over_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (isValidObject(_menu_label)) {
        lv_obj_add_flag(_menu_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void FlappyGameCard::showMenu() {
    _game_state = GameState::MENU;
    
    if (isValidObject(_menu_label)) {
        lv_obj_clear_flag(_menu_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (isValidObject(_game_over_label)) {
        lv_obj_add_flag(_game_over_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    hideGameElements();
}

void FlappyGameCard::hideGameElements() {
    if (isValidObject(_player_obj)) {
        lv_obj_add_flag(_player_obj, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (isValidObject(_score_label)) {
        lv_obj_add_flag(_score_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    for (auto* obstacle : _obstacles) {
        if (isValidObject(obstacle)) {
            lv_obj_add_flag(obstacle, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void FlappyGameCard::showGameElements() {
    if (isValidObject(_player_obj)) {
        lv_obj_clear_flag(_player_obj, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (isValidObject(_score_label)) {
        lv_obj_clear_flag(_score_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    for (auto* obstacle : _obstacles) {
        if (isValidObject(obstacle)) {
            lv_obj_clear_flag(obstacle, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    if (isValidObject(_menu_label)) {
        lv_obj_add_flag(_menu_label, LV_OBJ_FLAG_HIDDEN);
    }
    
    if (isValidObject(_game_over_label)) {
        lv_obj_add_flag(_game_over_label, LV_OBJ_FLAG_HIDDEN);
    }
}

bool FlappyGameCard::isValidObject(lv_obj_t* obj) const {
    return obj && lv_obj_is_valid(obj);
} 