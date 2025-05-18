#include "UltimaCard.h"
#include "hardware/Input.h" // For BUTTON_UP, BUTTON_DOWN, BUTTON_CENTER
#include "sprites/3buttondungeon/sprite_3buttondungeon.h" // Path for splash image

// Define the game font - using unscii_16 for all Ultima text
LV_FONT_DECLARE(lv_unscii_16_custom_symbols); // Declare the custom symbols font
static lv_font_t ultima_font_with_fallback;   // Static font object to hold main font + fallback
#define ULTIMA_GAME_FONT &ultima_font_with_fallback // Using unscii_16 with custom symbols fallback

// Fallback for LV_FONT_MONO if unscii_16 isn't designated as a mono font by LVGL
// (though for unscii, it effectively is)
#ifndef LV_FONT_MONO
#define LV_FONT_MONO ULTIMA_GAME_FONT
#endif

// Static variables for combo action debouncing
static unsigned long last_combo_action_time = 0;
const unsigned long COMBO_COOLDOWN_MS = 100; // Cooldown for combo actions (milliseconds)

UltimaCard::UltimaCard(uint16_t width, uint16_t height)
    : card_obj(nullptr),
      map_label(nullptr), stats_label(nullptr), message_label(nullptr),
      splash_screen_img(nullptr), // Initialize new member
      start_screen_title_label(nullptr), start_screen_instructions_label(nullptr),
      game_won_label(nullptr), // Initialize new member
      card_width(width), card_height(height),
      current_display_state(UltimaCardDisplayState::SHOWING_SPLASH_SCREEN) { // Default to splash screen
    // Game engine is initialized by its own constructor

    // Initialize the fallback font
    memcpy(&ultima_font_with_fallback, &lv_font_unscii_16, sizeof(lv_font_t));
    ultima_font_with_fallback.fallback = &lv_unscii_16_custom_symbols;
}

UltimaCard::~UltimaCard() {
    // LVGL handles deletion of children automatically
}

lv_obj_t* UltimaCard::createCard(lv_obj_t* parent) {
    card_obj = lv_obj_create(parent);
    lv_obj_remove_style_all(card_obj);
    lv_obj_set_size(card_obj, card_width, card_height);
    lv_obj_set_style_bg_color(card_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(card_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(card_obj, 2, 0);
    lv_obj_center(card_obj);

    // --- Splash Screen UI Element ---
    splash_screen_img = lv_img_create(card_obj);
    lv_img_set_src(splash_screen_img, &sprite_3buttondungeon);
    lv_obj_align(splash_screen_img, LV_ALIGN_CENTER, 0, 0);

    // --- Game UI Elements (created next, visibility managed by state) ---
    uint16_t game_message_area_height = 20; // Adjusted for potentially larger font
    uint16_t game_main_area_height = card_height - game_message_area_height - 4;
    uint16_t game_stats_area_width = 80; // Adjusted for potentially larger font
    uint16_t game_map_area_width = card_width - game_stats_area_width - 8; // Adjusted

    map_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(map_label, ULTIMA_GAME_FONT, 0);
    lv_obj_set_style_text_color(map_label, lv_color_white(), 0);
    lv_obj_set_size(map_label, game_map_area_width, game_main_area_height);
    lv_label_set_long_mode(map_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(map_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(map_label, LV_ALIGN_TOP_LEFT, 0, 0);

    stats_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(stats_label, ULTIMA_GAME_FONT, 0);
    lv_obj_set_style_text_color(stats_label, lv_color_hex(0xFFD700), 0);
    lv_obj_set_size(stats_label, game_stats_area_width, game_main_area_height);
    lv_label_set_long_mode(stats_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(stats_label, LV_ALIGN_TOP_RIGHT, 0, 0);

    message_label = lv_label_create(card_obj); // Common message label
    lv_obj_set_style_text_font(message_label, ULTIMA_GAME_FONT, 0);
    lv_obj_set_style_text_color(message_label, lv_color_hex(0xADD8E6), 0);
    lv_obj_set_size(message_label, card_width - 4, game_message_area_height - 2);
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    // Initial alignment/text set in setDisplayState

    // --- Start Screen UI Elements (Text based) ---
    // start_screen_title_label = lv_label_create(card_obj);
    // lv_obj_set_style_text_font(start_screen_title_label, ULTIMA_GAME_FONT, 0);
    // lv_obj_set_style_text_color(start_screen_title_label, lv_color_white(), 0);
    // lv_label_set_text(start_screen_title_label, "How to Play");
    // lv_obj_align(start_screen_title_label, LV_ALIGN_TOP_MID, 0, 5); // Adjusted y for larger font

    start_screen_instructions_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(start_screen_instructions_label, ULTIMA_GAME_FONT, 0);
    lv_obj_set_style_text_color(start_screen_instructions_label, lv_color_hex(0xCCCCCC), 0);
    lv_label_set_long_mode(start_screen_instructions_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(start_screen_instructions_label, card_width - 10); // Padded width
    lv_label_set_text(start_screen_instructions_label,
        "UP/DOWN: Move\n"
        "MID: Use\n"
        "MID+UP/DOWN: Right/Left\n"
        "UP+DOWN: Restart"
    );
    lv_obj_align(start_screen_instructions_label, LV_ALIGN_TOP_MID, 0, 5); // Adjusted y: move to where title was

    // --- Game Won Screen UI Element ---
    game_won_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(game_won_label, ULTIMA_GAME_FONT, 0); 
    lv_obj_set_style_text_color(game_won_label, lv_color_white(), 0);
    lv_label_set_text_fmt(game_won_label, "YOU WIN %s", T_YOU_WIN_SYMBOL);
    lv_obj_set_width(game_won_label, card_width - 10); 
    lv_obj_set_style_text_align(game_won_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(game_won_label, LV_ALIGN_CENTER, 0, 0);

    setDisplayState(UltimaCardDisplayState::SHOWING_SPLASH_SCREEN); // Initial state

    return card_obj;
}

void UltimaCard::setDisplayState(UltimaCardDisplayState new_state) {
    current_display_state = new_state;

    // Hide all toggleable elements first
    if(splash_screen_img) lv_obj_add_flag(splash_screen_img, LV_OBJ_FLAG_HIDDEN);
    // if(start_screen_title_label) lv_obj_add_flag(start_screen_title_label, LV_OBJ_FLAG_HIDDEN);
    if(start_screen_instructions_label) lv_obj_add_flag(start_screen_instructions_label, LV_OBJ_FLAG_HIDDEN);
    if(map_label) lv_obj_add_flag(map_label, LV_OBJ_FLAG_HIDDEN);
    if(stats_label) lv_obj_add_flag(stats_label, LV_OBJ_FLAG_HIDDEN);
    if(game_won_label) lv_obj_add_flag(game_won_label, LV_OBJ_FLAG_HIDDEN); // Hide initially

    switch (new_state) {
        case UltimaCardDisplayState::SHOWING_SPLASH_SCREEN:
            if(splash_screen_img) lv_obj_clear_flag(splash_screen_img, LV_OBJ_FLAG_HIDDEN);
            if(message_label) {
                lv_label_set_text(message_label, ""); // Set to empty string
                lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_align(message_label, LV_ALIGN_BOTTOM_MID, 0, -2);
            }
            break;
        case UltimaCardDisplayState::SHOWING_START_SCREEN:
            // if(start_screen_title_label) lv_obj_clear_flag(start_screen_title_label, LV_OBJ_FLAG_HIDDEN);
            if(start_screen_instructions_label) lv_obj_clear_flag(start_screen_instructions_label, LV_OBJ_FLAG_HIDDEN);
            if(message_label) {
                lv_label_set_text(message_label, ""); // Set to empty string
                lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_align(message_label, LV_ALIGN_BOTTOM_MID, 0, -2);
            }
            break;
        case UltimaCardDisplayState::SHOWING_GAME:
            if(map_label) lv_obj_clear_flag(map_label, LV_OBJ_FLAG_HIDDEN);
            if(stats_label) lv_obj_clear_flag(stats_label, LV_OBJ_FLAG_HIDDEN);
            if(message_label) {
                lv_label_set_text(message_label, "Welcome to the Desert!"); // Or restore last game message
                lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_LEFT, 0);
                lv_obj_align(message_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
            }
            updateView(); // Initial render of game map and stats
            break;
        case UltimaCardDisplayState::SHOWING_GAME_WON_SCREEN:
            if(game_won_label) lv_obj_clear_flag(game_won_label, LV_OBJ_FLAG_HIDDEN);
            if(message_label) { // Clear any game messages
                lv_label_set_text(message_label, "Press MID to Restart");
                lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_align(message_label, LV_ALIGN_BOTTOM_MID, 0, -2);
            }
            break;
    }
}

void UltimaCard::updateMapDisplay() {
    if (current_display_state != UltimaCardDisplayState::SHOWING_GAME) return;
    if (map_label) {
        String view = game_engine.renderView();
        lv_label_set_text(map_label, view.c_str());
    }
}

void UltimaCard::updateStatsDisplay() {
    if (current_display_state != UltimaCardDisplayState::SHOWING_GAME) return;
    if (stats_label) {
        String stats = game_engine.getFormattedStats();
        lv_label_set_text(stats_label, stats.c_str());
    }
}

void UltimaCard::updateView() {
    // Check for WIN state first
    if (game_engine.isGameWon()) {
        setDisplayState(UltimaCardDisplayState::SHOWING_GAME_WON_SCREEN);
        return;
    }
    // If not won, and not showing game, do nothing (handles splash/start screen)
    if (current_display_state != UltimaCardDisplayState::SHOWING_GAME) return;
    
    // If showing game, check for defeat
    if (game_engine.isPlayerDefeated()) { 
        lv_label_set_text(map_label, ""); 
        lv_label_set_text(stats_label, ""); 
        lv_label_set_text(message_label, "GAME OVER.\nTry again? (UP+DOWN).");
        lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(message_label, LV_ALIGN_CENTER, 0, 0);
        return;
    }
    // If game is active, not won, and not lost, update map and stats.
    updateMapDisplay();
    updateStatsDisplay();
}

bool UltimaCard::handleButtonPress(uint8_t button_index) {
    if (current_display_state == UltimaCardDisplayState::SHOWING_SPLASH_SCREEN) {
        if (button_index == Input::BUTTON_CENTER) {
            setDisplayState(UltimaCardDisplayState::SHOWING_START_SCREEN);
            return true; // Event handled by this card
        }
        return false; // Let CardNavigationStack handle UP/DOWN
    } else if (current_display_state == UltimaCardDisplayState::SHOWING_START_SCREEN) {
        if (button_index == Input::BUTTON_CENTER) {
            setDisplayState(UltimaCardDisplayState::SHOWING_GAME);
            return true; // Event handled
        }
        // For START_SCREEN, also let CardNavigationStack handle UP/DOWN if not CENTER
        return false; 
    }

    // Check for WIN state first
    if (game_engine.isGameWon()) {
        setDisplayState(UltimaCardDisplayState::SHOWING_GAME_WON_SCREEN);
        return true; // Handled
    }

    // Corrected: Check current_display_state for SHOWING_GAME_WON_SCREEN
    if (current_display_state == UltimaCardDisplayState::SHOWING_GAME_WON_SCREEN) {
        unsigned long current_time_ms = millis();
        bool up_currently_pressed = Input::isUpPressed();
        bool down_currently_pressed = Input::isDownPressed();

        // Allow CENTER or UP+DOWN to restart
        if (button_index == Input::BUTTON_CENTER || (up_currently_pressed && down_currently_pressed)) {
             if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) {
                game_engine.restartGame();
                setDisplayState(UltimaCardDisplayState::SHOWING_SPLASH_SCREEN);
                last_combo_action_time = current_time_ms;
                // updateView(); // Not strictly needed here as setDisplayState for splash will handle it
                return true; // Handled
            }
        }
        return true; // Absorb other inputs on win screen if no restart action taken
    }

    // --- Game Running Logic (current_display_state == SHOWING_GAME) ---
    // Note: The isGameWon() check is primarily in updateView() which transitions to SHOWING_GAME_WON_SCREEN.
    // If somehow handleButtonPress is called while game is won but state hasn't transitioned, 
    // it might be good practice to also check here, but it could lead to state transition issues if not careful.
    // For now, relying on updateView() to manage the transition to win screen.

    if (game_engine.isPlayerDefeated()) {
        // If player is defeated, only allow restart combo
        unsigned long current_time_ms = millis();
        bool up_currently_pressed = Input::isUpPressed();
        bool down_currently_pressed = Input::isDownPressed();
        if (up_currently_pressed && down_currently_pressed) {
            if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) { 
                game_engine.restartGame(); 
                setDisplayState(UltimaCardDisplayState::SHOWING_SPLASH_SCREEN); 
                last_combo_action_time = current_time_ms;
                updateView(); // Update to clear GAME OVER and show splash
            }
        }
        return true; // Absorb all other input if defeated
    }

    bool action_taken = false; // Flag to see if player action or combat happened
    // turn_message is now handled by game_engine, retrieve it after actions.

    unsigned long current_time_ms = millis();
    bool up_currently_pressed = Input::isUpPressed();
    bool down_currently_pressed = Input::isDownPressed();
    bool center_currently_pressed = Input::isCenterPressed();

    if (up_currently_pressed && down_currently_pressed) {
        if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) { 
            game_engine.restartGame(); 
            setDisplayState(UltimaCardDisplayState::SHOWING_SPLASH_SCREEN); 
            last_combo_action_time = current_time_ms;
            action_taken = true; // Restart is an action
        }
    } 
    else if (center_currently_pressed && up_currently_pressed) {
        if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) {
            game_engine.movePlayer(1, 0); // Move Right
            action_taken = true;
            last_combo_action_time = current_time_ms;
        }
    } else if (center_currently_pressed && down_currently_pressed) {
        if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) {
            game_engine.movePlayer(-1, 0); // Move Left
            action_taken = true;
            last_combo_action_time = current_time_ms;
        }
    }

    if (!action_taken && !center_currently_pressed) { // Avoid single center press triggering move and search
       if(button_index == Input::BUTTON_UP && !down_currently_pressed && !center_currently_pressed) {
            game_engine.movePlayer(0, -1); // Move Up
            action_taken = true;
       } else if (button_index == Input::BUTTON_DOWN && !up_currently_pressed && !center_currently_pressed) {
            game_engine.movePlayer(0, 1);  // Move Down
            action_taken = true;
       }
    } 
    
    if (!action_taken && button_index == Input::BUTTON_CENTER && !up_currently_pressed && !down_currently_pressed) {
        game_engine.searchCurrentTile(); // Search action populates its own part of turn_message
        action_taken = true;
    }

    if (action_taken && !game_engine.isPlayerDefeated()) { // Don't let monsters move if player just got defeated by their own action (e.g. trap)
        game_engine.moveMonsters(); // Monsters move after player action
    }

    String msg_to_display = game_engine.getTurnMessageAndClear();
    if (message_label) {
        if (game_engine.isPlayerDefeated()) {
            lv_label_set_text(message_label, "GAME OVER.\nTry again? (UP+DOWN).");
            lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(message_label, LV_ALIGN_CENTER, 0, 0);
        } else if (!msg_to_display.isEmpty()) {
            lv_label_set_text(message_label, msg_to_display.c_str());
            // Ensure consistent alignment for scrolling game messages
            lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_LEFT, 0); 
            lv_obj_align(message_label, LV_ALIGN_BOTTOM_LEFT, 0, 0); 
        } else if (current_display_state == UltimaCardDisplayState::SHOWING_GAME) {
             lv_label_set_text(message_label, ""); // Clear message if nothing to display
             // Ensure it's still aligned correctly for SHOWING_GAME state even if empty
             lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_LEFT, 0);
             lv_obj_align(message_label, LV_ALIGN_BOTTOM_LEFT, 0, 0); 
        }
    }
    
    updateView(); // Update map and stats (will also handle GAME OVER display if needed)
    return true; 
} 