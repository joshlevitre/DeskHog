#pragma once

#include <lvgl.h>
#include <string>
#include "ui/InputHandler.h"

class PomodoroCard : public InputHandler {
public:
    PomodoroCard(lv_obj_t* parent);
    ~PomodoroCard();

    lv_obj_t* getCard() { return _card; }
    bool handleButtonPress(uint8_t button_index) override;
    void updateDisplay();  // Make public for timer callback
    void updateTallyDisplay(); // Method to update the tally label

private:
    void startTimer();
    void stopTimer();
    void flashRainbow();
    void switchMode();
    bool isValidObject(lv_obj_t* obj) const;
    void executePostTimerEffects();
    static void effects_timer_cb(lv_timer_t* timer);
    static void anim_set_bg_color_cb(void * var, int32_t v); // Animation callback
    static void anim_ready_cb_restore_color(lv_anim_t *a);   // Animation ready callback to restore color

    // UI elements
    lv_obj_t* _card;
    lv_obj_t* _background;
    lv_obj_t* _label;
    lv_obj_t* _label_shadow;
    lv_obj_t* _tally_label;      // Label for the tally marks
    lv_timer_t* _timer;
    lv_timer_t* _effects_timer; // For one-shot effects

    // State
    bool _is_running;
    bool _is_work_mode;  // true for work (25min), false for break (5min)
    int _remaining_seconds;
    int _completed_work_sessions; // Counter for completed work sessions
    static constexpr int WORK_TIME = 25;    // 25 seconds for testing
    static constexpr int BREAK_TIME = 5;    // 5 seconds for testing

    // Static color definitions for rainbow effect
    static const uint32_t RAINBOW_COLORS[];
    static const size_t RAINBOW_COLORS_COUNT;
}; 