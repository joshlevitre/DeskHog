#include "UltimaCard.h"
#include "hardware/Input.h" // For BUTTON_UP, BUTTON_DOWN, BUTTON_CENTER

// For now, let's assume a default mono font. If not available, this needs adjustment.
// Ideally, enable LV_FONT_MONTSERRAT_10 or a specific mono font in lv_conf.h
#ifndef LV_FONT_MONO
#define LV_FONT_MONO LV_FONT_DEFAULT
#endif

// Static variables for combo action debouncing
static unsigned long last_combo_action_time = 0;
const unsigned long COMBO_COOLDOWN_MS = 100; // Cooldown for combo actions (milliseconds)

UltimaCard::UltimaCard(uint16_t width, uint16_t height)
    : card_obj(nullptr), game_label(nullptr), message_label(nullptr), card_width(width), card_height(height) {
}

UltimaCard::~UltimaCard() {
    if (card_obj) {
        // lv_obj_del(card_obj); // Card's parent (CardNavigationStack) should handle deletion
        // card_obj = nullptr;
    }
}

lv_obj_t* UltimaCard::createCard(lv_obj_t* parent) {
    card_obj = lv_obj_create(parent);
    lv_obj_set_size(card_obj, card_width, card_height);
    lv_obj_set_style_bg_color(card_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(card_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card_obj, 0, 0);
    lv_obj_set_style_pad_all(card_obj, 0, 0); // No padding for the card itself
    lv_obj_center(card_obj);

    game_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(game_label, &lv_font_montserrat_14, 0); // Reverted to Montserrat 14 for compilation.
                                                                    // Enable a mono font in lv_conf.h for best results.
    lv_obj_set_style_text_color(game_label, lv_color_white(), 0);
    lv_obj_set_width(game_label, card_width - 10); // Allow some padding within the label
    lv_obj_set_height(game_label, card_height -10);
    lv_label_set_long_mode(game_label, LV_LABEL_LONG_WRAP); // Wrap text if too long
    lv_obj_align(game_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(game_label, "Initializing Ultima...\n@"); // Initial text

    // Create the message label (e.g., at the bottom of the card)
    message_label = lv_label_create(card_obj);
    lv_obj_set_style_text_font(message_label, &lv_font_montserrat_14, 0); // Use same font for messages
    lv_obj_set_style_text_color(message_label, lv_color_white(), 0);
    lv_obj_set_width(message_label, card_width - 10); // Allow some padding
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(message_label, LV_ALIGN_BOTTOM_MID, 0, -5); // Align at bottom with a small offset
    lv_label_set_text(message_label, ""); // Initially empty

    updateView();
    return card_obj;
}

void UltimaCard::updateView() {
    if (game_label) {
        String view = game_engine.renderView();
        lv_label_set_text(game_label, view.c_str());
    }
    // Clear message label on general view update if desired, or handle timeouts separately
    // if (message_label) {
    //     lv_label_set_text(message_label, "");
    // }
}

bool UltimaCard::handleButtonPress(uint8_t button_index) {
    bool handled = false;
    String msg = ""; 
    unsigned long current_time_ms = millis();

    bool up_currently_pressed = Input::isUpPressed();
    bool down_currently_pressed = Input::isDownPressed();
    bool center_currently_pressed = Input::isCenterPressed();

    // Priority 1: Check for combo actions
    if (center_currently_pressed && up_currently_pressed) {
        if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) {
            game_engine.movePlayer(1, 0); // Move Right
            updateView();
            // lv_label_set_text(message_label, "Right (C+U)"); // Optional: for debugging combos
            lv_label_set_text(message_label, ""); // Clear message on move
            last_combo_action_time = current_time_ms;
        }
        handled = true; // A combo is active, consume the event from CardNavigationStack
    } else if (center_currently_pressed && down_currently_pressed) {
        if (current_time_ms - last_combo_action_time > COMBO_COOLDOWN_MS) {
            game_engine.movePlayer(-1, 0); // Move Left
            updateView();
            // lv_label_set_text(message_label, "Left (C+D)"); // Optional: for debugging combos
            lv_label_set_text(message_label, ""); // Clear message on move
            last_combo_action_time = current_time_ms;
        }
        handled = true; // A combo is active, consume the event from CardNavigationStack
    }

    // Priority 2: Process individual button actions IF NO COMBO WAS HANDLED/ACTIVE
    if (!handled) { // Only process if no combo keys are actively pressed together
        switch (button_index) {
            case Input::BUTTON_UP:
                game_engine.movePlayer(0, -1); // Move Up (decreasing Y)
                updateView();
                lv_label_set_text(message_label, ""); // Clear message on move
                handled = true;
                break;
            case Input::BUTTON_DOWN:
                game_engine.movePlayer(0, 1);  // Move Down (increasing Y)
                updateView();
                lv_label_set_text(message_label, ""); // Clear message on move
                handled = true;
                break;
            case Input::BUTTON_CENTER:
                msg = game_engine.searchCurrentTile();
                if (message_label) {
                    lv_label_set_text(message_label, msg.c_str());
                }
                // updateView(); // No need to update the whole map view for a search action
                handled = true; 
                break;
        }
    }
    return handled;
} 