#pragma once

#include <lvgl.h>
#include "../game/UltimaGame.h"
#include "InputHandler.h" // Assuming InputHandler.h is in the ui directory

class UltimaCard : public InputHandler {
public:
    UltimaCard(uint16_t width, uint16_t height);
    ~UltimaCard();

    lv_obj_t* createCard(lv_obj_t* parent);
    void updateView();

    // From InputHandler
    bool handleButtonPress(uint8_t button_index) override;

    lv_obj_t* getLvglObject() { return card_obj; }

private:
    UltimaGame game_engine;
    lv_obj_t* card_obj; // The main LVGL object for this card
    lv_obj_t* game_label; // LVGL label to display ASCII game view
    lv_obj_t* message_label; // LVGL label for search messages etc.

    uint16_t card_width;
    uint16_t card_height;
}; 