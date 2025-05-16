#include "UltimaCard.h"
#include "hardware/Input.h" // For BUTTON_UP, BUTTON_DOWN, BUTTON_CENTER

// Using unscii_8 font now, ensure it's enabled in lv_conf.h (LV_FONT_UNSCII_8 1)

// For now, let's assume a default mono font. If not available, this needs adjustment.
// Ideally, enable LV_FONT_MONTSERRAT_10 or a specific mono font in lv_conf.h
#ifndef LV_FONT_MONO
#define LV_FONT_MONO LV_FONT_DEFAULT
#endif

// Static variables for combo action debouncing
static unsigned long last_combo_action_time = 0;
const unsigned long COMBO_COOLDOWN_MS = 100; // Cooldown for combo actions (milliseconds)

UltimaCard::UltimaCard(uint16_t width, uint16_t height)
    : card_obj(nullptr), map_label(nullptr), stats_label(nullptr), message_label(nullptr), card_width(width), card_height(height) {
}

UltimaCard::~UltimaCard() {
    // LVGL handles deletion of children when card_obj is deleted by CardNavigationStack or CardController
}

lv_obj_t* UltimaCard::createCard(lv_obj_t* parent) {
    card_obj = lv_obj_create(parent);
    lv_obj_remove_style_all(card_obj); // Use this to have a clean slate for styling
    lv_obj_set_size(card_obj, card_width, card_height);
    lv_obj_set_style_bg_color(card_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(card_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(card_obj, 2, 0); // Small padding for the overall card
    lv_obj_center(card_obj);

    // Define approximate layout percentages/pixels
    uint16_t message_area_height = 12; // Height for the bottom message log (1 line of unscii_8 + padding)
    uint16_t main_area_height = card_height - message_area_height - 4; // -4 for card padding top/bottom
    uint16_t stats_area_width = 70; // Fixed width for stats panel
    uint16_t map_area_width = card_width - stats_area_width - 6; // -6 for card padding left/right & gap

    // Create Map Label (Left Area)
    map_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(map_label, &lv_font_unscii_8, 0); 
    lv_obj_set_style_text_color(map_label, lv_color_white(), 0); // Default text color
    lv_obj_set_size(map_label, map_area_width, main_area_height);
    lv_label_set_long_mode(map_label, LV_LABEL_LONG_CLIP); // Clip if too long, or WRAP if preferred
    lv_obj_align(map_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_text(map_label, "Loading Map...");

    // Create Stats Label (Right Area)
    stats_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(stats_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(stats_label, lv_color_white(), 0); // Default text color
    lv_obj_set_size(stats_label, stats_area_width, main_area_height);
    lv_label_set_long_mode(stats_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(stats_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_label_set_text(stats_label, "Stats:");

    // Create Message Label (Bottom Area)
    message_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(message_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(message_label, lv_color_white(), 0); // Default text color
    lv_obj_set_size(message_label, card_width - 4, message_area_height -2); // Full width, adjusted for padding
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_SCROLL_CIRCULAR); // Or CLIP
    lv_obj_align(message_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_label_set_text(message_label, "Welcome to Ultima!");

    updateView(); // Initial render of map and stats
    return card_obj;
}

void UltimaCard::updateMapDisplay() {
    if (map_label) {
        String view = game_engine.renderView();
        lv_label_set_text(map_label, view.c_str());
    }
}

void UltimaCard::updateStatsDisplay() {
    if (stats_label) {
        String stats = game_engine.getFormattedStats();
        lv_label_set_text(stats_label, stats.c_str());
    }
}

void UltimaCard::updateView() {
    updateMapDisplay();
    updateStatsDisplay();
    // Message label is updated by actions, not typically here unless clearing it.
}

bool UltimaCard::handleButtonPress(uint8_t button_index) {
    bool handled = false;
    String msg_text = ""; // Renamed from 'msg' to avoid conflict 
    unsigned long current_time_ms = millis();

    // String action_msg_color = "#ADD8E6"; // LightBlue for general messages

    bool up_currently_pressed = Input::isUpPressed();
    bool down_currently_pressed = Input::isDownPressed();
    bool center_currently_pressed = Input::isCenterPressed();

    // Priority 1: Check for combo actions
    if (center_currently_pressed && up_currently_pressed) {
        if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) {
            game_engine.movePlayer(1, 0); // Move Right
            updateMapDisplay(); // Only update map, stats don't change on move
            msg_text = "Moved Right."; 
            last_combo_action_time = current_time_ms;
        }
        handled = true; 
    } else if (center_currently_pressed && down_currently_pressed) {
        if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) {
            game_engine.movePlayer(-1, 0); // Move Left
            updateMapDisplay(); // Only update map
            msg_text = "Moved Left.";
            last_combo_action_time = current_time_ms;
        }
        handled = true; 
    }

    if (!handled) {
        switch (button_index) {
            case Input::BUTTON_UP:
                game_engine.movePlayer(0, -1); // Move Up
                updateMapDisplay();
                msg_text = "Moved Up.";
                handled = true;
                break;
            case Input::BUTTON_DOWN:
                game_engine.movePlayer(0, 1);  // Move Down
                updateMapDisplay();
                msg_text = "Moved Down.";
                handled = true;
                break;
            case Input::BUTTON_CENTER:
                msg_text = game_engine.searchCurrentTile();
                handled = true; 
                break;
        }
    }

    if (message_label && !msg_text.isEmpty()) {
        lv_label_set_text(message_label, msg_text.c_str());
    }
    // Consider clearing message_label after a delay or on next *different* action if it gets too cluttered.

    return handled;
} 